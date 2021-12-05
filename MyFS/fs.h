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
#define MIN_VOLUME_SIZE 22600
#define LOGGEDIN 1
#define BADPW 0
#define UNLOGGEDIN -1


enum class FileType { Regular, Directory, Symlink };

struct Link {
    int inode_block_id; 
    char filename[FILENAME_MAX_LENGTH + 1];
};

struct INode final {
    FileType type;
    int n_links;
    int size;
    int data_block_ids[BLOCKS_PER_INODE];
};


class MyFS {
public:
    //string PATH_SEPARATOR = "/";
    //const std::string ROOTDIR_NAME = "/";
    int ZERO_BLOCK = -1;
    int BAD_BLOCK = -2;

    int rootInodeId = -1;
    int volumeSize = -1l;
    int bitmapBlocks = -1;
    int dataBlocks = -1;
    string cwd = "/";
    int isPwSetted = false;
    bool formatFirstTime = false;
    bool isLoggedIn = false;
    bool formated = false;
    fstream volume;


    //FUNCTION==================================================
    bool accessVolume(string name);
    bool formatMyFS(int size);
    bool createMyFS(int size);
    bool mount(const std::string& filename);
    void umount();
    bool importFile(string path);
    string list(const string& dirname);
    int create(const std::string& path, FileType type = FileType::Regular);
    bool fileExists(const std::string& filename);
    bool remove(const std::string& path);
    bool exportFile(string filename, string path);
    bool changePW(string pw);
    int authentication();
    bool checkPW(string pw);
    string hashCode(string pw);
    /*bool mkdir(const std::string& dirname);
    bool link(const std::string& target, const std::string& name_path);
    bool cd(const std::string& dirname);
    std::string ls();
    bool rmdir(const std::string& dirname);
    std::string pwd();
    bool symlink(const std::string& target, const std::string& name);*/


    //UTILITY==========================================
    //bool block_used(int block_id);
    //bool block_mark_used(int block_id);
    //void block_mark_unused(int block_id);
    //int div_ceil(int a, int b);//

    void writeToBlock(int block_id, const char* data, int size = BLOCK_SIZE, int shift = 0);
    void writeToBlock(int block_id, const INode* inode);
    void readFromBlock(int block_id, char* data, int size = BLOCK_SIZE, int shift = 0);//
    void readFromBlock(int block_id, INode* inode);
    int getUnusedBlock();
    int getInodeByLinks(int inode_block_id, int max_follows = MAX_SYMLINK_FOLLOWS);
    int findInodeBlock(const string& path);
    int findFileInode(int block_id, const string& filename);
    void removeInode(int inode_id);

    //string join_path(string part1, string part2);   //
    //string get_file_directory(const string& path);  //
    //string get_filename(const string& path);        //


    //FILE
    void readFile(char* data, int size, int shift, int block_id);
    int sizeFile(int block_id);
    bool resizeFile(int size, int block_id);
    bool writeFile(const char* data, int size, int shift, int block_id);
    string catFile(int block_id);

};