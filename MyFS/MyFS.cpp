#include "fs.h"

#include <iostream>

using namespace std;

int main() {
    Myfs* myfs = new Myfs();
    while (true) {
        cin.clear();
        cout << myfs->cwd << ">>> ";
        string cmd;
        cin >> cmd;
        if (cin.eof()) {
            break;
        }
        if (cmd == "mount") {
            string fsFileName;
            cin >> fsFileName;
            if (myfs->mount(fsFileName)) {
                cout << "File system mounted!" << endl;
            }
            else {
                cout << "Cannot mount file system!" << endl;
            }
        }
        else if (cmd == "umount") {
            myfs->umount();
            cout << "File system unmounted!" << endl;
        }
        else {
            cout << "Uknown command!" << endl;
        }
    }
    myfs->umount();
    return EXIT_SUCCESS;
}
