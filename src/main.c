#include "../header/header.h"

int main() {
    char cmd[100];
    loadUser();
    loadGroup();
    DirectoryTree* miniOS = loadDirectory();
    loginUser = login();
    Directory* loginDirectory = findRoute(loginUser->dir);
    miniOS->home = loginDirectory;
    miniOS->current = loginDirectory;

    while(1) {
        printHeader(miniOS, loginUser);
        fgets(cmd, sizeof(cmd), stdin);
        cmd[strlen(cmd)-1] = '\0';
        classificationCommand(cmd);
    }

    return 0;
}