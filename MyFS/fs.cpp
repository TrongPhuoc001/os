#include "fs.h"

uint32_t MyFS::sig = 0x36160524;


Date::Date() {
	time_t t = time(NULL);
	tm* now = new tm();
	localtime_s(now, &t);
	this->mtime = (uint8_t)(now->tm_hour * 60 + now->tm_min);
	this->day = (uint8_t)now->tm_mday;
	this->month = (uint8_t)now->tm_mon;
	this->year = (uint8_t)(now->tm_year+1900-2000);
}


MyFS::MyFS() {
	this->f = NULL;
	this->sate = "Empty";
	this->size = 0;
}

bool MyFS::mount(string path) {
	fstream f;
	f.open(path, ios::binary | ios::in | ios::out);
	Header h;
	Date now;

	h.sig = this->sig;
	h.data_size = 0;
	h.modtime = now;
	h.central_size = 0;
	h.pass_len = 0;
	f.seekg(ios::end);
	int size = f.tellg();
	int num_blocks = floor((ceil(size / BLOCK_SIZE)-7)/CLUSTER);
	int table_size = ceil(num_blocks * 0.005);
	f.seekp(ios::beg);
	f.write((char*)&h, sizeof(Header));
	this->f = &f;
	this->sate = path;
	return true;
}

bool MyFS::import(string path)
{
	ifstream fi;
	fi.open(path, ios::binary | ios::ate);
	if (!fi) {
		cout << "File doesn't exist" << endl;
		return false;
	}
	uint32_t fi_size = (uint32_t)fi.tellg();

	uint16_t flag;
	string password = "";
	cout << "Do you want password (0 for no, 1 for yes)"; cin >> flag;
	if (flag == 1)
	{
		cout << "Create password for this file:"; cin >> password;
	}


	
}
void MyFS::list() {

}


bool MyFS::changePass(string pass) {
	fstream f;
	
	f.open("MyFS.Dat", ios::binary | ios::in | ios::out);
	if (!f) {
		return false;
	}
	f.seekg(sizeof(Header) - 16);
	uint16_t pass_len = pass.length();
	cout << pass_len << endl;
	f.write((char*)&pass_len, sizeof(uint16_t));
	f.write(&pass[0], pass_len);
	f.close();
	return true;
}

bool MyFS::open(string filename, string password) {
	fstream f;

	f.open(filename, ios::binary|ios::in|ios::out);
	if (!f) {
		return false;
	}
	f.seekg(0);
	f.read((char*)&this->h, sizeof(Header));
	if (this->h.sig != this->sig) {
		cout << "Faile sig" << endl;
		return false;
	}
	if (password != h.password) {
		cout << "Wrong password" << endl;
		f.close();
		return false;
	}
	f.seekg(ios::end);
	this->size = f.tellg();
	this->sate = filename;
	this->f = &f;
	return true;
}
bool MyFS::make(string password, uint32_t size) {
	Header h;
	Date now;

	size = size * 1024 * 1024;
	uint16_t num_block = size / 512;
	
	uint16_t pass_len = (uint16_t)password.length();
	ofstream f;
	f.open(MYFS, ios::binary);

	f.write((char*)&h, sizeof(Header));
	f.seekp(size-1);
	f.write(new char, 1);
	f.close();
	return true;
}

void MyFS::readBlock(char* buffer, uint16_t offset, int numblks) {
	f->seekg(0);
	f->seekg(offset * (uint32_t)BLOCK_SIZE);
	f->read(buffer, (uint32_t)numblks * (uint32_t)BLOCK_SIZE);
	return;
}
bool MyFS::writeBlock(char* buffer, uint16_t offset, uint32_t size) {
	try {
		f->seekg(offset * BLOCK_SIZE);
		f->write(buffer, size);
		return true;
	}
	catch(...){
		return false;
	}
	
}
void MyFS::print() {

}



