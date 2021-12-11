#include "fs.h"
#include<fstream>
#pragma warning(disable:4996)

//FUNCTION==========================================================
int MyFS::authentication() {
    int tempBlockId = findInodeBlock("pw"); // Tìm block chứa tên file (trong cây thư mục chính)
    int block_id = getInodeByLinks(tempBlockId);// Tìm block chứa dữ liệu của file
    string content = catFile(block_id);
    content.size();
    if (content.size() != 0 && !isLoggedIn)return UNLOGGEDIN;
    if (content.size() == 0) return BADPW;
    else return LOGGEDIN;
}
bool MyFS::accessVolume(string name) {
    formated = true;
    mount(name);
    return true;
}
bool MyFS::formatMyFS(int size, string disk) {
    if(!createMyFS(size, disk))return false;
    if(!mount(disk+":\\MyFS.dat"))return false;
    return true;
}
bool MyFS::createMyFS(int size, string disk) {
    fstream myfs;

    myfs.open(disk+":\\MyFS.dat", ios_base::out, ios_base::binary);
    if (myfs.fail()) return false;

    char buffer[1000] = { '0' };
    if (size < MIN_VOLUME_SIZE)return false;

    for (int i = 0; i < size / 1000; i++) {
        if(!myfs.write(buffer, 1000)) return false;
    }
    return true;
    
}

bool MyFS::mount(const std::string& filename)
{
    umount();
    formated = true;
    //mở file MYFS.DAT để format lại cấu trúc bên trong
    volume.open(filename, fstream::in | fstream::binary | fstream::out);
    if (volume.fail()) {
        return false;
    }

    // Kiểm tra dung lượng của file MYFS.DAT
    volume.seekg(0, volume.end);
    volumeSize = volume.tellg();

    // Tính số block mà bitmap chiếm
    int bitmapBlockSize = BLOCK_SIZE * BLOCK_SIZE * 8;
    if (volumeSize == 0) {
        bitmapBlocks = 0;
        dataBlocks = 0;
    }
    else {
        bitmapBlocks = (volumeSize - 1) / bitmapBlockSize + 1;
        dataBlocks = (volumeSize - 1) / BLOCK_SIZE + 1;
    }

    // Sử dụng block đầu tiên ngay sau bitmap để làm cây thư mục
    rootInodeId = bitmapBlocks;

    volume.seekg((rootInodeId - bitmapBlocks) / 8, volume.beg);
    int checkBit = (volume.get() & (1 << ((rootInodeId - bitmapBlocks) % 8)));
    int bit = checkBit != 0;

    // Nếu MYFS.DAT chưa được format trước đây thì sẽ được format từ đầu
    if (!bit) {
        cout << "Format first time\n";
        formatFirstTime = true;
        // Đi tới bit đánh dấu trạng thái của block của cây thư mục chính sẽ chiếm (hiên tại bit đang là 0 vì chưa được format)
        volume.seekg((rootInodeId - bitmapBlocks) / 8, volume.beg);
        auto old_mask = volume.get();

        // Bật bit này lên là 1 (đánh dấu block này đang bị chiếm)
        volume.seekp((rootInodeId - bitmapBlocks) / 8, volume.beg);
        volume.put(static_cast<char>(old_mask | (1 << ((rootInodeId - bitmapBlocks) % 8))));

        volume.flush();

        // Tạo một inode để cấu hình các thông tin cho block của cây thư mục chính
        INode rootInode;
        rootInode.n_links = 1;
        rootInode.size = 0;
        rootInode.type = FileType::Directory;

        // Viết dữ liêu của inode vào block của cây thư mục chính
        writeToBlock(rootInodeId, &rootInode);

        // Tạo một block để lưu mật khẩu
        create("pw");
    }
    return true;
}

void MyFS::umount()
{
    formated = false;

    volumeSize = -1;
    bitmapBlocks = -1;
    dataBlocks = -1;
    volume.close();
}

int MyFS::create(const string& path, FileType type) {
    // Tìm block đang trống
    int inodeBlockId = getUnusedBlock();
    if (inodeBlockId == BAD_BLOCK) {
        return BAD_BLOCK;
    }

    // Đi tới bit trong bảng bitmap đang giữ trạng thái của block trống vừa tìm được
    volume.seekg((inodeBlockId - bitmapBlocks) / 8, volume.beg);
    char oldMap = volume.get();

    // Bật bit này lên là 1 (đánh dấu block này đang bị chiếm)
    volume.seekp((inodeBlockId - bitmapBlocks) / 8, volume.beg);
    volume.put(static_cast<char>(oldMap | (1 << ((inodeBlockId - bitmapBlocks) % 8))));
    volume.flush();

    // Kiểm tra độ dài của tên file (nếu dài quá giới hạn sẽ trả về BAD_BLOCK)
    if (path.size() > FILENAME_MAX_LENGTH) {
        return BAD_BLOCK;
    }

    // Tăng dung lượng của cây thư mục để lưu giữ tên file
    int block_id = 1;
    int old_dir_size = sizeFile(block_id);//Lấy dung lượng cũ của cây thư mục
    resizeFile(old_dir_size + sizeof(Link), block_id);// Tăng dung lượng của cây thư mục


    // Create link in root directory
    Link lnk;
    lnk.inodeBlockId = inodeBlockId;
    strcpy(lnk.filename, path.c_str());
    writeFile(reinterpret_cast<char*>(&lnk), sizeof(Link), old_dir_size, 1);

    // Tạo 1 inode để lưu thông tin của file vừa được tạo (tên file, địa chỉ block chứa dữ liệu của file,......)
    INode inode;
    inode.size = 0;
    inode.n_links = 1;
    inode.type = type;

    //Viết dữ liệu của inode vừa được tạo vào block trống tìm được
    writeToBlock(inodeBlockId, &inode);
    return inodeBlockId;
}

bool MyFS::fileExists(const string& filename) {
    return findInodeBlock(filename) != BAD_BLOCK;
}

bool MyFS::importFile(string path) {
    // Tìm đến file cần được import để đọc dữ liệu
    fstream fi(path, fstream::in | fstream::binary | fstream::out);
    if (fi.fail()) {
        return false;
    }

    char* buffer = nullptr;
    int len = 0;

    // Khi truy cập tới file thành công 
    if (fi) {
        // Lấy độ dài của file để tạo 1 buffer lữu dữ liêu
        fi.seekg(0, fi.end);
        int length = fi.tellg();
        len = length;
        fi.seekg(0, fi.beg);

        // Tạo buffer để chép dữ liệu của file
        buffer = new char[length];

        // Chép dữ liệu file vô buffer
        fi.read(buffer, length);
        fi.close();

        for (int i = 0; i < length; i++) {
            cout << buffer[i];
        }
        cout << endl;
    }

    // Trích xuất tên của file cần được import trong đường dẫn
    int idx = path.find_last_of('\\');
    string filename = path.substr(idx + 1);

    // Nếu tên file đã tồn tại trong MYFS.DAT thì không import được 
    if (fileExists(filename)) {
        cout << "File da ton tai!" << endl;
        return false;
    }

    // Tạo file với tên được trích xuất
    create(filename);
    string data = "";
    for (int i = 0; i < len; i++) {
        data = data + buffer[i];
    }

    // Tìm block chứa thông tin của file vừa được tạo
    int tempBlockId = findInodeBlock(filename);
    int blockId = getInodeByLinks(tempBlockId);

    // Viết dữ liệu của file cần được import vào file vừa được tạo
    resizeFile(data.size(), blockId);
    if (writeFile(data.data(), data.size(), 0, blockId)) {
        return true;
    }

    // Nếu xảy ra lỗi trong quá trình viết dữ liệu thì xóa file đó đi và trả về false
    else {
        remove(filename);
        return false;
    }
}

bool MyFS::remove(const string& path) {

    // Tìm block chứa thông tin của file cần xóa 
    int inode = findInodeBlock(path);
    if (inode == BAD_BLOCK) {
        return false;
    }


    //auto dirname = get_file_directory(path);
    //auto filename = get_filename(path);

    if (path.size() > FILENAME_MAX_LENGTH) {
        return false;
    }

    int blockId = getInodeByLinks(1);
    /*int old_dir_size = size(block_id);
    resizeFile(old_dir_size + sizeof(Link), block_id);*/

    // Lấy dữ liệu đang có ở trong cây thư mục chính (để thực hiện xóa thông tin của file cần xóa)
    auto systemData = catFile(blockId);
    vector<char> data(systemData.begin(), systemData.end());
    int dirSize = data.size();

    // Duyệt qua từng inode trong block của cây thư mục để tìm tên file 
    for (int n_file = 0; n_file < dirSize / sizeof(Link); ++n_file) {
        auto& lnk = *reinterpret_cast<Link*>(data.data() + n_file * sizeof(Link));

        // khi tìm thấy inode chứa tên file, thực hiện xóa inode đó đi và cập nhật lại kích thước của cây thư mục chính
        if (lnk.filename == path) {
            removeInode(lnk.inodeBlockId); // xóa inode chứa thông tin của file cần xóa

            // Cập nhật lại kích thước của cây thư mục chính
            readFile(reinterpret_cast<char*>(&lnk), sizeof(Link), dirSize - sizeof(Link), blockId);
            writeFile(reinterpret_cast<char*>(&lnk), sizeof(Link), n_file * sizeof(Link), blockId);
            resizeFile(dirSize - sizeof(Link), blockId);
            return true;
        }
    }
    return false;
}

string MyFS::list(const string& dirname) {
    // Tìm block chứa thông tin của cây thư mục chính
    int blockId = getInodeByLinks(1);

    // Lấy kích thước của cây thư mục chính
    int dirSize  = sizeFile(blockId);
    vector<char> data(static_cast<size_t>(dirSize));
    
    // Đọc dữ liệu của cây thư mục chính (là thông tin của tất cả file bên trong)
    readFile(data.data(), dirSize, 0, blockId);
    string result;
    string parts = "";

    // Duyệt qua từng thông tin file và lưu vào một chuỗi để in ra màn hình
    for (int n_file = 0; n_file < dirSize / sizeof(Link); ++n_file) {
        auto& lnk = *reinterpret_cast<Link*>(data.data() + n_file * sizeof(Link));
        parts = lnk.filename;
        if (parts == "pw")continue;
        result += parts;
        result += '\n';
    }

    // Kết quả trả về là chuỗi chứa thông tin của tất cả các file 
    return result;
}

bool MyFS::exportFile(string filename, string path) {
    fstream fi;

    // Tạo 1 file trống tại vị trí cần export 
    fi.open(path, ios_base::binary | ios_base::out);
    if (fi.fail()) {
        return false;
    }
    // Tìm file cần export ra bên ngoài
    if (fileExists(filename)) {
        int tempBlockId = findInodeBlock(filename); // Tìm block chứa tên file (trong cây thư mục chính)
        int blockId = getInodeByLinks(tempBlockId);// Tìm block chứa dữ liệu của file
        string content = catFile(blockId); // Lấy dữ liệu của file ra một chuỗi
        char* buffer = new char[content.size()];
        for (int i = 0; i < content.size(); i++) {
            buffer[i] = content[i];
        }
        if (!fi.write(buffer, content.size())) return false;// Ghi chuỗi nội dung file vào file vừa được tạo bên ngoài
    }
    fi.close();
    return true;
}

bool MyFS::checkPW(string pw) {
    // Tìm block chứa password
    int tempBlockId = findInodeBlock("pw");
    int blockId = getInodeByLinks(tempBlockId);

    // Hash password được truyền vô để so sánh với password được lưu bên trong
    string hashPw = hashCode(pw);
    string oldPw = catFile(blockId);
    if (oldPw == hashPw) {
        isLoggedIn = true;
        return true;
    }
    return false;
}

bool MyFS::changePW(string new_pw) {
    // Hash password tăng tính bảo mật
    string hashPw = hashCode(new_pw);
    
    // Tìm block được quy định để lưu password
    int tempBlockId = findInodeBlock("pw");
    int blockId = getInodeByLinks(tempBlockId);

    // Tăng kích thước của block
    resizeFile(hashPw.size(), blockId);

    // Viết password vào block được quy định
    isPwSetted = writeFile(hashPw.data(), hashPw.size(), 0, blockId) ? true : false;
    return isPwSetted;
}

string MyFS::hashCode(string pw) {
    hash<string> mystdhash;
    return to_string(mystdhash(pw));
}


void MyFS::writeToBlock(int block_id, const char* data, int size, int shift)
{
    volume.seekp(block_id * BLOCK_SIZE + shift, volume.beg);
    volume.write(data, size);
    volume.flush();
}

void MyFS::writeToBlock(int block_id, const INode* inode)
{
    writeToBlock(block_id, reinterpret_cast<const char*>(inode));
}


void MyFS::readFromBlock(int block_id, char* data, int size, int shift) {
    volume.seekg(block_id * BLOCK_SIZE + shift, volume.beg);
    volume.read(data, size);
    volume.flush();
}

void MyFS::readFromBlock(int block_id, INode* inode) {
    readFromBlock(block_id, reinterpret_cast<char*>(inode));
}

int MyFS::getUnusedBlock() {
    // Duyệt qua từng bit trong bảng bitmap để tìm bit nào chưa được bật lên 1 (bit này đánh dấu block có vị trí tương ứng chưa bị chiếm)
    // Duyệt qua từng block bitmap (có thể có nhiều hơn 1 block bitmap)
    for (int bitmapBlockId = 0; bitmapBlockId < bitmapBlocks; ++bitmapBlockId) {
        char data[BLOCK_SIZE];
        readFromBlock(bitmapBlockId, data);
        // Duyệt qua từng byte trong block bitmap thứ bitmask_block_id
        for (int idx = 0; idx < BLOCK_SIZE; ++idx) {
            if (data[idx] != ~'\0') 
            {
                // Duyệt qua từng bit trong byte thứ idx
                for (int n_bit = 0; n_bit < 8; ++n_bit) {
                    if ((data[idx] & (1 << n_bit)) == 0) // Kiểm tra xem bit thứ n_bit có đang được bật lên 1 hay vẫn là 0
                    {
                        // Nếu bit chưa được bật lên 1 thì trả về vị trí của block tương ứng với bit này
                        int result = (bitmapBlockId * BLOCK_SIZE + idx) * 8 + n_bit;
                        if (result >= dataBlocks) {
                            return BAD_BLOCK;
                        }
                        result = result + bitmapBlocks;
                        return result;
                    }
                }
            }
        }
    }

    return BAD_BLOCK;
}

int MyFS::getInodeByLinks(int inodeBlockId, int max_follows) {
    INode inode;
    readFromBlock(inodeBlockId, &inode);
    if (inode.type != FileType::Symlink) {
        return inodeBlockId;
    }
    else {
        if (max_follows == 0) {
            return BAD_BLOCK;
        }
        int blockId = getInodeByLinks(inodeBlockId);
        auto target = catFile(blockId);
        auto linkInode = findInodeBlock(target);
        if (linkInode == BAD_BLOCK) {
            return BAD_BLOCK;
        }
        return getInodeByLinks(findInodeBlock(target), max_follows - 1);
    }
}

int MyFS::findInodeBlock(const string& path) {
    int dirInode = rootInodeId;
    size_t start = 1;
    return findFileInode(dirInode, path);
}

int MyFS::findFileInode(int block_id, const string& filename) {
    auto data = catFile(block_id);
    auto dirSize = data.size();

    for (int n_file = 0; n_file < dirSize / sizeof(Link); ++n_file) {
        auto& lnk = *reinterpret_cast<const Link*>(data.data() + n_file * sizeof(Link));
        if (lnk.filename == filename) {
            return lnk.inodeBlockId;
        }
    }
    return BAD_BLOCK;
}


//FILE==========================================================
void MyFS::readFile(char* data, int size, int shift, int block_id) {
    INode inode;
    readFromBlock(block_id, &inode);
    int index = 0;
    while (size > 0) {
        int block_index = shift / BLOCK_SIZE;
        int& block_id = inode.data_block_ids[block_index];
        int s = min(size, ((block_index + 1) * BLOCK_SIZE) - shift);
        if (block_id != ZERO_BLOCK) {
            readFromBlock(block_id, data + index, s, shift % BLOCK_SIZE);
        }
        else {
            fill(data + index, data + index + s, '\0');
        }
        shift += s;
        size -= s;
        index += s;
    }
}

int MyFS::sizeFile(int block_id) {
    INode inode;
    readFromBlock(block_id, &inode);
    return inode.size;
}

string MyFS::catFile(int block_id) {
    vector<char> data(static_cast<size_t>(sizeFile(block_id)));
    readFile(data.data(), data.size(), 0, block_id);
    string result(data.begin(), data.end());
    return result;
}

bool MyFS::resizeFile(int size, int block_id) {
    INode inode;

    //đọc dữ liệu cũ của block
    readFromBlock(block_id, &inode);
    if (size == inode.size) return true;

    //tính số inode cũ mà file đang sử dụng 
    int oldBlocks = inode.size == 0 ? 0 : (inode.size - 1) / BLOCK_SIZE + 1;

    //tính số inode mới mà file sẽ sử dụng
    int blocks = size == 0 ? 0 : (size - 1) / BLOCK_SIZE + 1;

    //nếu số inode mới cần sử dụng bé hơn số inode cũ đang sử dụng thì trả về trạng thái chưa sử dụng cho inode k còn dùng tới
    if (blocks < oldBlocks) {
        for (int block_index = blocks; block_index < oldBlocks; ++block_index) {
            if (inode.data_block_ids[block_index] >= 0) {
                //trả lại trạng thái chưa sử dụng cho inode ko còn dùng tới 
                volume.seekg((inode.data_block_ids[block_index] - bitmapBlocks) / 8, volume.beg);
                auto  old_mask = volume.get();
                volume.seekp((inode.data_block_ids[block_index] - bitmapBlocks) / 8, volume.beg);
                volume.put(static_cast<char>(old_mask & ~(1 << ((inode.data_block_ids[block_index] - bitmapBlocks) % 8))));
                volume.flush();
            }
        }
    }
    //nếu số inode mới cần dùng lớn hơn hoặc bằng số inode cũ thì cập nhật dữ liệu của dữ liệu mới vào ngay sau các dữ liệu của block cũ
    else {
        if (inode.size % BLOCK_SIZE != 0 && inode.data_block_ids[oldBlocks - 1] != ZERO_BLOCK) {
            char dataLastParts[BLOCK_SIZE];
            //đọc dữ liệu của block cũ vào 1 buffer
            readFromBlock(inode.data_block_ids[oldBlocks - 1], dataLastParts);

            //ghép dữ liệu mới vào dữ liệu cũ
            fill(dataLastParts + inode.size % BLOCK_SIZE, dataLastParts + BLOCK_SIZE, '\0');

            //chép dữ liệu được ghép vào block
            writeToBlock(inode.data_block_ids[oldBlocks - 1], dataLastParts);
        }
        fill(inode.data_block_ids + oldBlocks, inode.data_block_ids + blocks, ZERO_BLOCK);
    }

    inode.size = size;
    writeToBlock(block_id, &inode);
    return true;
}

bool MyFS::writeFile(const char* data, int size, int shift, int block_id) {
    INode inode;
    readFromBlock(block_id, &inode);
    int index = 0;
    bool isInodeUpdated = false;
    while (size > 0) {
        int nextBlock = shift / BLOCK_SIZE;
        int& nextBlockId = inode.data_block_ids[nextBlock];
        if (nextBlockId == ZERO_BLOCK) {
            nextBlockId = getUnusedBlock();
            if (nextBlockId == BAD_BLOCK) {
                inode.size = shift;
                writeToBlock(block_id, &inode);
                return false;
            }
            volume.seekg((nextBlockId - bitmapBlocks) / 8, volume.beg);
            char old_mask = volume.get();
            volume.seekp((nextBlockId - bitmapBlocks) / 8, volume.beg);
            volume.put(static_cast<char>(old_mask | (1 << ((nextBlockId - bitmapBlocks) % 8))));
            volume.flush();

            isInodeUpdated = true;
        }
        int s = min(size, ((nextBlock + 1) * BLOCK_SIZE) - shift);
        writeToBlock(nextBlockId, data + index, s, shift % BLOCK_SIZE);

        shift += s;
        size -= s;
        index += s;
    }
    if (isInodeUpdated) {
        writeToBlock(block_id, &inode);
    }
    return true;
}

void MyFS::removeInode(int inode_id) {
    INode inode;
    readFromBlock(inode_id, &inode);

    if (inode.n_links == 1) {
        for (int blockIndex = 0; blockIndex * BLOCK_SIZE < inode.size; ++blockIndex) {
            int block_id = inode.data_block_ids[blockIndex];
            if (block_id != ZERO_BLOCK) {
                //trả về trạng thái chưa sử dụng cho các inode 
                volume.seekg((block_id - bitmapBlocks) / 8, volume.beg);
                auto  old_mask = volume.get();
                volume.seekp((block_id - bitmapBlocks) / 8, volume.beg);
                volume.put(static_cast<char>(old_mask & ~(1 << ((block_id - bitmapBlocks) % 8))));
                volume.flush();
            }
        }
        //trả về trạng thái chưa sử dụng cho các inode 
        volume.seekg((inode_id - bitmapBlocks) / 8, volume.beg);
        auto  old_mask = volume.get();
        volume.seekp((inode_id - bitmapBlocks) / 8, volume.beg);
        volume.put(static_cast<char>(old_mask & ~(1 << ((inode_id - bitmapBlocks) % 8))));
        volume.flush();
    }
    else {
        --inode.n_links;
        writeToBlock(inode_id, &inode);
    }
}
