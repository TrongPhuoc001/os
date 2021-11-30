#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <ctime>
#include <cmath>
#include <unordered_map>

using namespace std;

#define BLOCK_SIZE 512
#define CLUSTER 4
#define MYFS "MyFS.Dat"
#define SIGNATURE 0x36160524
#define FILENAME_MAX_LEN 16
#define PASSWORD_MAX_LEN 24

class Date {
public:
	uint8_t mtime;
	uint8_t day;
	uint8_t month;
	uint8_t year;

	Date();
};

struct Header {
	uint32_t sig;
	uint32_t data_size;
	Date modtime;
	uint16_t central_size;
	uint16_t offset_backup_start;
	uint16_t pass_len;
	char password[14];
};

struct central {
	uint32_t check_sum;
	uint16_t type;
	Date date;
	uint16_t offset;
	uint32_t size;
	uint16_t fn_len;
	char filename[14];
	uint16_t pass_len;
	char password[14];
};
class MyFS {
public:
	static uint32_t sig;
	Header h;
	uint32_t size;
	string password;
	bool state;
	fstream* f;

public:

	bool open(string filename, string password);
	bool make(string password, uint32_t size);
	bool import(string path);
	void list();
	bool verify(string pass);
	bool changePass(string pass);
	void print();
	void readBlock(char* buffer, uint16_t offset, int numblks);
	bool writeBlock(char* buffer, uint16_t offset, uint32_t size);
};

