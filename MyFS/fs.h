#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cmath>

using namespace std;

#define BLOCK_SIZE  512 // 64 (512)
#define FILENAME_MAX_LENGTH  15
#define PASSWORD_MAX_LENGTH 15
#define BLOCKS_PER_INODE  126 // 14 (126)
#define MAX_SYMLINK_FOLLOWS  10

enum class FileType { Regular, Directory, Symlink };
struct Link {
    int inode_block_id; // relative to the last bitmask block
    char filename[FILENAME_MAX_LENGTH + 1];
};
struct INode final {
    FileType type;
    int n_links;
    int size;
    int data_block_ids[BLOCKS_PER_INODE];
    // todo add link to additional data_blocks
};


class Myfs {
public:
    string PATH_SEPARATOR = "/";
    const std::string ROOTDIR_NAME = "/";
    int ZERO_BLOCK = -1;
    int BAD_BLOCK = -2;

    int root_inode_id = -1;
    int device_capacity = -1l;
    int n_bitmask_blocks = -1;
    int n_data_blocks = -1;
    string cwd = ROOTDIR_NAME;
    fstream fio;

    bool mount(const std::string& filename);
    void umount();
    std::string ls();
    int create(const std::string& path, FileType type = FileType::Regular);
    bool link(const std::string& target, const std::string& name_path);
    bool unlink(const std::string& path);
    bool file_exists(const std::string& filename);
    bool mkdir(const std::string& dirname);
    bool rmdir(const std::string& dirname);
    bool cd(const std::string& dirname);
    std::string pwd();
    bool symlink(const std::string& target, const std::string& name);

    bool block_used(int block_id);
    bool block_mark_used(int block_id);
    void write_block(int block_id, const char* data, int size = BLOCK_SIZE, int shift = 0);
    void write_block(int block_id, const INode* inode);
};