#include "fs.h"



bool Myfs::mount(const std::string& filename)
{
    umount();
    fio.open(filename, fstream::in | fstream::binary | fstream::out);
    if (fio.fail()) {
        return false;
    }

    // measure device capacity  
    fio.seekg(0, fio.end);
    device_capacity = fio.tellg();
    cout << device_capacity << endl;
    // measure how many blocks are used for bitmaskz
    n_bitmask_blocks = ceil((device_capacity-1)/(BLOCK_SIZE * BLOCK_SIZE * 8)+1);
    n_data_blocks = ceil((device_capacity-1)/ BLOCK_SIZE +1);
    root_inode_id = n_bitmask_blocks; // use the first block after bitmask
    cout << root_inode_id << endl;
    // if first time (device not formatted)
    if (!block_used(root_inode_id)) {
        // FORMAT IT! (via creating root directory)
        block_mark_used(root_inode_id);

        INode root_inode;
        root_inode.n_links = 1;
        root_inode.size = 0;
        root_inode.type = FileType::Directory;
        write_block(root_inode_id, &root_inode);
    }

    return true;    
}

void Myfs::umount()
{
    device_capacity = -1;
    n_bitmask_blocks = -1;
    n_data_blocks = -1;
    fio.close();
}

bool Myfs::block_used(int block_id)
{   
    fio.seekg((block_id - n_bitmask_blocks) / 8, fio.beg);
    return (fio.get() & (1 << ((block_id - n_bitmask_blocks) % 8))) != 0;
}

bool Myfs::block_mark_used(int block_id)
{
    fio.seekg((block_id - n_bitmask_blocks) / 8, fio.beg);
    char old_mask = fio.get();
    fio.seekp((block_id - n_bitmask_blocks) / 8, fio.beg);
    fio.put(static_cast<char>(old_mask | (1 << ((block_id - n_bitmask_blocks) % 8))));
    fio.flush();
    return true;
}

void Myfs::write_block(int block_id, const char* data, int size = BLOCK_SIZE, int shift = 0)
{
    fio.seekp(block_id * BLOCK_SIZE + shift, fio.beg);
    fio.write(data, size);
    fio.flush();
}

void Myfs::write_block(int block_id, const INode* inode)
{
    write_block(block_id, reinterpret_cast<const char*>(inode));
}
