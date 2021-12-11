#include "fs.h"
#include <iostream>

using namespace std;

void run() {
    MyFS* myfs = new MyFS();
    bool insideVolume = false;
    while (true) {
        if (!insideVolume) {
            cout << "Nhap \"format\" de tao volume MyFS.dat\n";
            cout << "Nhap \"access\" + \"ten volume\" de truy cap toi volume\n";
        }
        cout << "\n>>> ";
        string cmd;
        cin >> cmd;
        if (!myfs->formated) {
            while (cmd != "access" && cmd != "format") {
                cout << "Nhap \"format\" de tao volume MyFS.dat\n";
                cout << "Nhap \"access\" + \"ten volume\" de truy cap toi volume\n";
                cout << "\n>>> ";
                cin >> cmd;
            }
        }

        if (myfs->formated) {
            int auth = myfs->authentication();
            if (auth == BADPW) {
                while (!myfs->isPwSetted) {
                    string pw;
                    cout << "Volume chua co mat khau, vui long tao mat khau\n";
                    cout << "Nhap mat khau cua ban: ";
                    cin >> pw;
                    cout << "Xac nhan mat khau: ";
                    string cpw;
                    cin >> cpw;
                    if (pw == cpw) {
                        myfs->changePW(pw);
                    }
                    else {
                        cout << "Tao mat khau that bai\n";
                    }
                }
            }
            if (auth == UNLOGGEDIN) {
                bool check = false;
                while (!check) {
                    cout << "Ban chua dang nhap, vui long dang nhap de truy xuat volume\n";
                    string pw;
                    cout << "Nhap mat khau: ";
                    cin >> pw;
                    check = myfs->checkPW(pw);
                    if (!check) cout << "Sai mat khau\n";
                }
                cout << "Dang nhap thanh cong\n";
                cout << "Neu lan dau su dung, nhap \"help\" de biet cac lenh truy xuat volume\n";
            }
        }
        if (cin.eof()) {
            break;
        }
        if (cmd == "format") {
            string disk = "";
            cout << "Nhap o dia luu tru MyFS.dat (D, E, C, ..): ";
            cin >> disk;
            int size;
            cout << "Nhap kich thuoc volume:";
            cin >> size;
            if (myfs->formatMyFS(size, disk)) {
                cout << "Volume format thanh cong!" << endl;
                insideVolume = true;
            }
            else {
                cout << "Khong the format volume!" << endl;
                cout << "Dam bao rang kich thuoc volume lon hon 5120MB" << endl;
            }
        }
        if (cmd == "access") {
            string fsFileName;
            cin >> fsFileName;
            if (myfs->mount(fsFileName)) {
                cout << "Truy cap volume thanh cong!" << endl;
                insideVolume = true;
            }
            else {
                cout << "Khong the truy cap volume\n" << endl;
            }
        }
        else if (cmd == "chpw") {
            string pw;
            cout << "Nhap mat khau cu:";
            cin >> pw;
            if (!myfs->checkPW(pw)) {
                cout << "Sai mat khau\n";
            }
            else {
                cout << "Nhap mat khau moi:";
                cin >> pw;
                if (myfs->changePW(pw)) {
                    cout << "Doi mat khau thanh cong\n";
                }
                else cout << "Doi mat khau that bai\n";
            }
        }
        else if (cmd == "ls") {
            cout << myfs->list(myfs->cwd);
        }
        else if (cmd == "rm") {
            string filename;
            cin >> filename;
            if (!myfs->fileExists(filename)) {
                cout << "File khong ton tai" << endl;
            }
            else {
                cout << (myfs->remove(filename) ? "Xoa file thanh cong" : "Xoa file that bai") << endl;
            }
        }
        else if (cmd == "import") {
            string filename;
            cin >> filename;
            if (myfs->importFile(filename)) {
                cout << "Import file thanh cong\n";
            }
            else {
                cout << "Khong the chep du lieu\n";
                cout<<"(dam bao volume con trong cho kich thuoc du lieu hoac duong dan dung)" << endl;
            }
        }
        else if (cmd == "export") {
            string filename;
            string path;
            cin >> filename;
            cin >> path;
            if (myfs->exportFile(filename, path)) {
                cout << "Export file thanh cong\n";
            }
            else cout << "Export file that bai\n";
        }
        else if (cmd == "help") {
            cout << "1. Nhap \"format\" de tao volume MyFS.dat (luu y: neu MyFS.dat da duoc tao, neu tao lai voi kich thuoc khac se bi loi\n\n";
            cout << "2. Nhap \"access\" + \"ten volume\" de truy cap toi volume (luu y: neu truy cap toi volume chua duoc format se bi loi)\n\n";
            cout << "3. Nhap \"chpw\" de doi mat khau\n\n";
            cout << "4. Nhap \"ls\" de liet ke cac tap tin trong volume\n\n";
            cout << "5. Nhap \"rm\" + \"ten file\" xoa tap tin khoi volume\n\n";
            cout << "6. Nhap \"import\" + \"duong dan\" de import tap tin ben ngoai volume\n\n";
            cout << "7. Nhap \"export\" + \"ten file\" + \"duong dan\" de export file ra ben ngoai volume\n\n";
        }
        else {
            cout << "~~~~~~~~~~~~~~~~~~" << endl;
        }
    }
    myfs->umount();
}


int main() {
    run();
    return EXIT_SUCCESS;
}
