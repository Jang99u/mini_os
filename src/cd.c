#include "../header/header.h"

// Change the current working directory
void changeDirectory(char* path) {
    if(path == NULL || strcmp(path, "") == 0) {
        // 경로가 입력되지 않은 경우 홈 디렉토리로 이동
        dirTree->current = dirTree->home;
        return;
    } else if(strcmp(path, "~") == 0) {
        dirTree->current = dirTree->home;
        return;
    }

    char* routeOrigin = (char*)malloc(100 * sizeof(char));;
    strcpy(routeOrigin, path);
    
    Directory* targetDirectory = findRoute(path);

    if(targetDirectory == NULL) {
        printf("cd: No such file or directory: %s\n", routeOrigin);
        return;
    } else {
        if(targetDirectory->type == '-') {
            printf("cd: not a directory: %s\n", routeOrigin);
            return;
        } else {
            dirTree->current = targetDirectory;
            return;
        }
    }
}