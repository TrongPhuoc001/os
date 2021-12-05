#include "fs.h"
#include<fstream>
#pragma warning(disable:4996)

//FUNCTION==========================================================
int MyFS::authentication() {
    int temp_block_id = findInodeBlock("pw"); // Tìm block chứa tên file (trong cây thư mục chính)
    int block_id = getInodeByLinks(temp_block_id);// Tìm block chứa dữ liệu của file
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
bool MyFS::formatMyFS(int size) {
    if(!createMyFS(size))return false;
    if(!mount("D:\\MyFS.dat"))return false;
    return true;
}
bool MyFS::createMyFS(int size) {
    fstream myfs;

    myfs.open("D:\\MyFS.dat", ios_base::out, ios_base::binary);
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
    int bitmask_block_size = BLOCK_SIZE * BLOCK_SIZE * 8;
    if (volumeSize == 0) {
        bitmapBlocks = 0;
        dataBlocks = 0;
    }
    else {
        bitmapBlocks = (volumeSize - 1) / bitmask_block_size + 1;
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
        INode root_inode;
        root_inode.n_links = 1;
        root_inode.size = 0;
        root_inode.type = FileType::Directory;

        // Viết dữ liêu của inode vào block của cây thư mục chính
        writeToBlock(rootInodeId, &root_inode);

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
    /*if (findInodeBlock(path) != BAD_BLOCK) {
        return BAD_BLOCK;
    }*/

    // Tìm block đang trống
    int inode_block_id = getUnusedBlock();
    if (inode_block_id == BAD_BLOCK) {
        return BAD_BLOCK;
    }

    //block_mark_used(inode_block_id);
    // Đi tới bit trong bảng bitmap đang giữ trạng thái của block trống vừa tìm được
    volume.seekg((inode_block_id - bitmapBlocks) / 8, volume.beg);
    char old_mask = volume.get();

    // Bật bit này lên là 1 (đánh dấu block này đang bị chiếm)
    volume.seekp((inode_block_id - bitmapBlocks) / 8, volume.beg);
    volume.put(static_cast<char>(old_mask | (1 << ((inode_block_id - bitmapBlocks) % 8))));
    volume.flush();

    // Kiểm tra độ dài của tên file (nếu dài quá giới hạn sẽ trả về BAD_BLOCK)
    if (path.size() > FILENAME_MAX_LENGTH) {
        return BAD_BLOCK;
    }

    // Tăng dung lượng của cây thư mục để lưu giữ tên file
    //int block_id = getInodeByLinks(1);// Tìm địa chỉ block của cây thư mục
    int block_id = 1;
    int old_dir_size = sizeFile(block_id);//Lấy dung lượng cũ của cây thư mục
    resizeFile(old_dir_size + sizeof(Link), block_id);// Tăng dung lượng của cây thư mục


    // Create link in root directory
    Link lnk;
    lnk.inode_block_id = inode_block_id;
    strcpy(lnk.filename, path.c_str());
    writeFile(reinterpret_cast<char*>(&lnk), sizeof(Link), old_dir_size, 1);

    // Tạo 1 inode để lưu thông tin của file vừa được tạo (tên file, địa chỉ block chứa dữ liệu của file,......)
    INode inode;
    inode.size = 0;
    inode.n_links = 1;
    inode.type = type;

    //Viết dữ liệu của inode vừa được tạo vào block trống tìm được
    writeToBlock(inode_block_id, &inode);
    return inode_block_id;
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
    int temp_block_id = findInodeBlock(filename);
    int block_id = getInodeByLinks(temp_block_id);

    // Viết dữ liệu của file cần được import vào file vừa được tạo
    resizeFile(data.size(), block_id);
    if (writeFile(data.data(), data.size(), 0, block_id)) {
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
    int target_inode = findInodeBlock(path);
    if (target_inode == BAD_BLOCK) {
        return false;
    }


    //auto dirname = get_file_directory(path);
    //auto filename = get_filename(path);

    if (path.size() > FILENAME_MAX_LENGTH) {
        return false;
    }

    int block_id = getInodeByLinks(1);
    /*int old_dir_size = size(block_id);
    resizeFile(old_dir_size + sizeof(Link), block_id);*/

    // Lấy dữ liệu đang có ở trong cây thư mục chính (để thực hiện xóa thông tin của file cần xóa)
    auto s_data = catFile(block_id);
    vector<char> data(s_data.begin(), s_data.end());
    int old_dir_size = data.size();

    // Duyệt qua từng inode trong block của cây thư mục để tìm tên file 
    for (int n_file = 0; n_file < old_dir_size / sizeof(Link); ++n_file) {
        auto& lnk = *reinterpret_cast<Link*>(data.data() + n_file * sizeof(Link));

        // khi tìm thấy inode chứa tên file, thực hiện xóa inode đó đi và cập nhật lại kích thước của cây thư mục chính
        if (lnk.filename == path) {
            removeInode(lnk.inode_block_id); // xóa inode chứa thông tin của file cần xóa

            // Cập nhật lại kích thước của cây thư mục chính
            readFile(reinterpret_cast<char*>(&lnk), sizeof(Link), old_dir_size - sizeof(Link), block_id); 
            writeFile(reinterpret_cast<char*>(&lnk), sizeof(Link), n_file * sizeof(Link), block_id);
            resizeFile(old_dir_size - sizeof(Link), block_id);
            return true;
        }
    }
    return false;
}

string MyFS::list(const string& dirname) {
    // Tìm block chứa thông tin của cây thư mục chính
    int block_id = getInodeByLinks(1);

    // Lấy kích thước của cây thư mục chính
    int dir_size = sizeFile(block_id);
    vector<char> data(static_cast<size_t>(dir_size));
    
    // Đọc dữ liệu của cây thư mục chính (là thông tin của tất cả file bên trong)
    readFile(data.data(), dir_size, 0, block_id);
    string result;
    string parts = "";

    // Duyệt qua từng thông tin file và lưu vào một chuỗi để in ra màn hình
    for (int n_file = 0; n_file < dir_size / sizeof(Link); ++n_file) {
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
        int temp_block_id = findInodeBlock(filename); // Tìm block chứa tên file (trong cây thư mục chính)
        int block_id = getInodeByLinks(temp_block_id);// Tìm block chứa dữ liệu của file
        string content = catFile(block_id); // Lấy dữ liệu của file ra một chuỗi
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
    int temp_block_id = findInodeBlock("pw");
    int block_id = getInodeByLinks(temp_block_id);

    // Hash password được truyền vô để so sánh với password được lưu bên trong
    string hash_pw = hashCode(pw);
    string old_pw = catFile(block_id);
    if (old_pw == hash_pw) {
        isLoggedIn = true;
        return true;
    }
    return false;
}

bool MyFS::changePW(string new_pw) {
    // Hash password tăng tính bảo mật
    string hash_pw = hashCode(new_pw);
    
    // Tìm block được quy định để lưu password
    int temp_block_id = findInodeBlock("pw");
    int block_id = getInodeByLinks(temp_block_id);

    // Tăng kích thước của block
    resizeFile(hash_pw.size(), block_id);

    // Viết password vào block được quy định
    isPwSetted = writeFile(hash_pw.data(), hash_pw.size(), 0, block_id) ? true : false;
    return isPwSetted;
}

string MyFS::hashCode(string pw) {
    hash<string> mystdhash;
    return to_string(mystdhash(pw));
}

//UTILITY==========================================================

//bool MyFS::block_used(int block_id)
//{   
//    volume.seekg((block_id - bitmapBlocks) / 8, volume.beg);
//    return (volume.get() & (1 << ((block_id - bitmapBlocks) % 8))) != 0;
//}

//bool MyFS::block_mark_used(int block_id)
//{
//    volume.seekg((block_id - bitmapBlocks) / 8, volume.beg);
//    char old_mask = volume.get();
//    volume.seekp((block_id - bitmapBlocks) / 8, volume.beg);
//    volume.put(static_cast<char>(old_mask | (1 << ((block_id - bitmapBlocks) % 8))));
//    volume.flush();
//    return true;
//}

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

//void MyFS::block_mark_unused(int block_id) {
//    volume.seekg((block_id - bitmapBlocks) / 8, volume.beg);
//    auto  old_mask = volume.get();
//    volume.seekp((block_id - bitmapBlocks) / 8, volume.beg);
//    volume.put(static_cast<char>(old_mask & ~(1 << ((block_id - bitmapBlocks) % 8))));
//    volume.flush();
//}

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
    for (int bitmask_block_id = 0; bitmask_block_id < bitmapBlocks; ++bitmask_block_id) {
        char data[BLOCK_SIZE];
        readFromBlock(bitmask_block_id, data);
        // Duyệt qua từng byte trong block bitmap thứ bitmask_block_id
        for (int idx = 0; idx < BLOCK_SIZE; ++idx) {
            if (data[idx] != ~'\0') 
            {
                // Duyệt qua từng bit trong byte thứ idx
                for (int n_bit = 0; n_bit < 8; ++n_bit) {
                    if ((data[idx] & (1 << n_bit)) == 0) // Kiểm tra xem bit thứ n_bit có đang được bật lên 1 hay vẫn là 0
                    {
                        // Nếu bit chưa được bật lên 1 thì trả về vị trí của block tương ứng với bit này
                        int result = (bitmask_block_id * BLOCK_SIZE + idx) * 8 + n_bit;
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

int MyFS::getInodeByLinks(int inode_block_id, int max_follows) {
    INode inode;
    readFromBlock(inode_block_id, &inode);
    if (inode.type != FileType::Symlink) {
        return inode_block_id;
    }
    else {
        if (max_follows == 0) {
            return BAD_BLOCK;
        }
        int block_id = getInodeByLinks(inode_block_id);
        auto target_name = catFile(block_id);
        auto linked_inode_block_id = findInodeBlock(target_name);
        if (linked_inode_block_id == BAD_BLOCK) {
            return BAD_BLOCK;
        }
        return getInodeByLinks(findInodeBlock(target_name), max_follows - 1);
    }
}

int MyFS::findInodeBlock(const string& path) {
    //const string absolute_path = path[0] != '/' ? join_path(cwd, path) : path;
    /*if (absolute_path == ROOTDIR_NAME) {
        return root_inode_id;
    }*/
    int dir_inode = rootInodeId;
    size_t start = 1;
    return findFileInode(dir_inode, path);

   /* while (true) {
        auto sep_index = absolute_path.find('/', start);
        if (sep_index == string::npos) {
            auto filename = absolute_path.substr(start);
            return findFileInode(dir_inode, filename);
        }
        else {
            auto subdirname = absolute_path.substr(start, sep_index - start);
            start = sep_index + 1;
            if (subdirname == ".") continue;
            dir_inode = findFileInode(dir_inode, subdirname);
            if (dir_inode == BAD_BLOCK) {
                return BAD_BLOCK;
            }
        }
    }*/
}

int MyFS::findFileInode(int block_id, const string& filename) {
    auto data = catFile(block_id);
    auto dir_size = data.size();

    for (int n_file = 0; n_file < dir_size / sizeof(Link); ++n_file) {
        auto& lnk = *reinterpret_cast<const Link*>(data.data() + n_file * sizeof(Link));
        if (lnk.filename == filename) {
            return lnk.inode_block_id;
        }
    }
    return BAD_BLOCK;
}

//string MyFS::join_path(string part1, string part2) {
//    if (part1 == ROOTDIR_NAME) {
//        return '/' + part2;
//    }
//    return part1 + '/' + part2;
//}

//string MyFS::get_file_directory(const string& path) {
//    const auto sep_index = path.find_last_of('/');
//    if (sep_index == string::npos) {
//        return cwd;
//    }
//    else {
//        auto dirname = path.substr(0, sep_index);
//        auto abs_dirname = dirname[0] == '/' ? dirname : join_path(cwd, dirname);
//        return abs_dirname;
//    }
//}

//string MyFS::get_filename(const string& path) {
//    const auto sep_index = path.find_last_of('/');
//    if (sep_index == string::npos) {
//        return path;
//    }
//    else {
//        return path.substr(sep_index + 1);
//    }
//}

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
            // zero data optimization (only nulls in file block)
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
    int n_old_blocks = inode.size == 0 ? 0 : (inode.size - 1) / BLOCK_SIZE + 1;

    //tính số inode mới mà file sẽ sử dụng
    int n_blocks = size == 0 ? 0 : (size - 1) / BLOCK_SIZE + 1;

    //nếu số inode mới cần sử dụng bé hơn số inode cũ đang sử dụng thì trả về trạng thái chưa sử dụng cho inode k còn dùng tới
    if (n_blocks < n_old_blocks) {
        for (int block_index = n_blocks; block_index < n_old_blocks; ++block_index) {
            if (inode.data_block_ids[block_index] >= 0) {
                //block_mark_unused(inode.data_block_ids[block_index]);
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
        if (inode.size % BLOCK_SIZE != 0 && inode.data_block_ids[n_old_blocks - 1] != ZERO_BLOCK) {
            char tail_data[BLOCK_SIZE];
            //đọc dữ liệu của block cũ vào 1 buffer
            readFromBlock(inode.data_block_ids[n_old_blocks - 1], tail_data);

            //ghép dữ liệu mới vào dữ liệu cũ
            fill(tail_data + inode.size % BLOCK_SIZE, tail_data + BLOCK_SIZE, '\0');

            //chép dữ liệu được ghép vào block
            writeToBlock(inode.data_block_ids[n_old_blocks - 1], tail_data);
        }
        fill(inode.data_block_ids + n_old_blocks, inode.data_block_ids + n_blocks, ZERO_BLOCK);
    }

    inode.size = size;
    writeToBlock(block_id, &inode);
    return true;
}

bool MyFS::writeFile(const char* data, int size, int shift, int block_id) {
    INode inode;
    readFromBlock(block_id, &inode);
    int index = 0;
    bool inode_updated = false;
    while (size > 0) {
        int next_block_index = shift / BLOCK_SIZE;
        int& next_block_id = inode.data_block_ids[next_block_index];
        if (next_block_id == ZERO_BLOCK) {
            next_block_id = getUnusedBlock();
            if (next_block_id == BAD_BLOCK) {
                inode.size = shift;
                writeToBlock(block_id, &inode);
                return false;
            }
            //block_mark_used(next_block_id);

            volume.seekg((next_block_id - bitmapBlocks) / 8, volume.beg);
            char old_mask = volume.get();
            volume.seekp((next_block_id - bitmapBlocks) / 8, volume.beg);
            volume.put(static_cast<char>(old_mask | (1 << ((next_block_id - bitmapBlocks) % 8))));
            volume.flush();

            inode_updated = true;
        }
        int s = min(size, ((next_block_index + 1) * BLOCK_SIZE) - shift);
        writeToBlock(next_block_id, data + index, s, shift % BLOCK_SIZE);

        shift += s;
        size -= s;
        index += s;
    }
    if (inode_updated) {
        writeToBlock(block_id, &inode);
    }
    return true;
}

void MyFS::removeInode(int inode_id) {
    INode inode;
    readFromBlock(inode_id, &inode);

    if (inode.n_links == 1) {
        for (int block_index = 0; block_index * BLOCK_SIZE < inode.size; ++block_index) {
            int block_id = inode.data_block_ids[block_index];
            if (block_id != ZERO_BLOCK) {
                //block_mark_unused(block_id);
                //trả về trạng thái chưa sử dụng cho các inode 
                volume.seekg((block_id - bitmapBlocks) / 8, volume.beg);
                auto  old_mask = volume.get();
                volume.seekp((block_id - bitmapBlocks) / 8, volume.beg);
                volume.put(static_cast<char>(old_mask & ~(1 << ((block_id - bitmapBlocks) % 8))));
                volume.flush();
            }
        }
        //block_mark_unused(inode_id);
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
