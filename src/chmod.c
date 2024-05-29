#include "../header/header.h"

void* changeMode(void* arg) {
    ChmodArgs* args = (ChmodArgs*)arg;
    char* path = args->path;
    char* mode = args->mode;

    Directory* targetDirectory = findRoute(path);
    if (targetDirectory == NULL) {
        printf("chmod: cannot access '%s': No such file or directory\n", path);
    } else {
        setPermission(targetDirectory, mode);
    }

    free(arg);
    pthread_exit(NULL);
}