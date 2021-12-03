#include "fs.h"
#include <iostream>

using namespace std;

void run() {
    MyFS* myfs = new MyFS();
    while (true) {
        if (myfs->formated) {
            int auth = myfs->authentication();
            if (auth == BADPW) {
                while (!myfs->pw_set) {
                    string pw;
                    cout << "Create your password: ";
                    cin >> pw;
                    cout << "Confirm your password: ";
                    string cpw;
                    cin >> cpw;
                    if (pw == cpw) {
                        myfs->change_pw(pw);
                    }
                    else {
                        cout << "Create password fail\n";
                    }
                }
            }
            if (auth == UNLOGGEDIN) {
                bool check = false;
                while (!check) {
                    string pw;
                    cout << "Enter your password: ";
                    cin >> pw;
                    check = myfs->check_pw(pw);
                    if (!check) cout << "Wrong password\n";
                }
                cout << "Loggin succesfully";

            }
        }
        cout << endl;
        cout << ">>> ";
        string cmd;
        cin >> cmd;
        if (cin.eof()) {
            break;
        }
        if (cmd == "format") {
            int size;
            cout << "Enter volume size:";
            cin >> size;
            if (myfs->formatMyFS(size)) {
                cout << "File system formated!" << endl;
            }
            else {
                cout << "Cannot format file system!" << endl;
                cout << "Make sure your volume size is bigger than 226000" << endl;
            }
        }
        if (cmd == "access") {
            string fsFileName;
            cin >> fsFileName;
            if (myfs->mount(fsFileName)) {
                cout << "Volume access!" << endl;
            }
            else {
                cout << "Can not access volume!" << endl;
            }
        }
        else if (cmd == "chpw") {
            string pw;
            cout << "enter your password: ";
            cin >> pw;
            if (!myfs->pw_set) {
                cout << "confirm your password: ";
                string cpw;
                cin >> cpw;
                if (pw == cpw) {
                    myfs->change_pw(pw);
                }
                else {
                    cout << "Create password fail\n";
                }
            }
            else {
                if (!myfs->check_pw(pw)) {
                    cout << "wrong password\n";
                }
                else {
                    cout << "enter new password: ";
                    cin >> pw;
                    if (myfs->change_pw(pw)) {
                        cout << "change password successfully\n";
                    }
                    else cout << "change password fail\n";
                }
            }
        }
        else if (cmd == "umount") {
            myfs->umount();
            cout << "File system unmounted!" << endl;
        }
        else if (cmd == "ls") {
            cout << myfs->ls(myfs->cwd);
        }
        else if (cmd == "rm") {
            string filename;
            cin >> filename;
            if (!myfs->file_exists(filename)) {
                cout << "File doesn't exist" << endl;
            }
            else {
                cout << (myfs->unlink(filename) ? "Hard link was removed" : "Hard link wasn't removed") << endl;
            }
        }
        else if (cmd == "import") {
            string filename;
            cin >> filename;
            if (myfs->importFile(filename)) {
                cout << "Import file successfully\n";
            }
            else {
                cout << "Cannot write data (probably not enough space)" << endl;
            }
        }
        else if (cmd == "export") {
            string filename;
            string path;
            cin >> filename;
            cin >> path;
            if (myfs->exportFile(filename, path)) { 
                cout << "Export file successfully\n"; 
            }
            else cout << "Export file failed\n";
        }
        else {
            cout << "Uknown command!" << endl;
        }
    }
    myfs->umount();
}
int main() {
    run();
    return EXIT_SUCCESS;
}
