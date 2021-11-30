#include "fs.h"


int main() {
	MyFS* vfs = new MyFS();
	string root = "";
	string password = vfs->password;
	cout << "make | open | list | import | repass | delete | quit | help" << endl;
	while (true) {
		std::cout << std::endl << root + ">>> ";
		std::string cmd;
		std::cin >> cmd;
		if (cmd == "list") {
			cout << "All file show here";
		}
		else if (cmd == "make") {
			string filename;
			cin >> filename;
			int size;
			cout << "size (mb):";
			cin >> size;
			char flag;
			string password;
			cout << "Do u want password (y/n):"; cin >> flag;
			if (flag == 'y') {
				cout << "passoword:"; cin >> password;
				vfs->make(password, size);
			}
			else {
				vfs->make("", size);
			}
			cout << "make " << filename << " success" << endl;
		}
		else if (cmd == "open") {
			string filename;
			cout << "file name: ";cin >> filename;
			cout << "open " << filename;
		}
		else if (cmd =="repass") {
			string repassword;
			string newpassword;
			string renewpass;
			cout << "Your current password:"; cin >> repassword;
			if (repassword != password) {
				cout << "wrong password"; 
				continue;
			}
			
			cout << "New password:"; cin >> newpassword;
			cout << "Retype new password:"; cin >> renewpass;
			if (newpassword != renewpass) {
				cout << "retype doesn't match";
				continue;
			}
			else {
				if (vfs->changePass(newpassword)) {
					cout << "Change password success" << endl;
					password = newpassword;
				}
				else {
					cout << "Error when changing password" << endl;
				}
			}
		}
		else if(cmd=="import"){
			string filename;
			cin >> filename;
			if (vfs->import(filename)) {
				cout << "import success" << endl;
			}
			else {
				cout << "File doesn't exist" << endl;
			}
		}
		else if (cmd == "list") {
			vfs->list();
		}
		else if (cmd == "quit") {
			break;
		}
		else if (cmd == "help") {
			std::cout << "list | open | import | repass | delete | quit | help" << std::endl;
		}
		else {
			std::cout << "Command not found" << std::endl;
		}
	}
	return 0;
}
