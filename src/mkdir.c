#include "../header/header.h"

// Create a new directory
Directory* createNewDirectory(char* name, const char* mode) {
    Directory* newDir = (Directory*)malloc(sizeof(Directory));
    strncpy(newDir->name, name, MAX_NAME);
    newDir->type = 'd';
    newDir->visible = true;

    // 권한 설정
    setPermission(newDir, mode);
    newDir->UID = loginUser->UID;
    newDir->GID = loginUser->GID;
    newDir->size = 4096; // 기본 크기 설정
    setDirectoryTime(newDir);
    newDir->parent = NULL;
    newDir->leftChild = NULL;
    newDir->rightSibling = NULL;

    return newDir;
}

// Add directory route to new directory
void addDirectoryRoute(Directory* newDir, Directory* parent, char* dirName) {
    char name[MAX_ROUTE];

    if(strcmp(parent->route, "/") == 0) {
        strcpy(name, parent->route);
        strcat(name, dirName);
        strncpy(newDir->route, name, MAX_ROUTE);
    } else {
        strcpy(name, parent->route);
        strcat(name, "/");
        strcat(name, dirName);
        strncpy(newDir->route, name, MAX_ROUTE);
    }
}

// Thread function to create a new directory
void* makeDirectory(void* arg) {
    Directory* returnDirectory;
    MkdirArgs* args = (MkdirArgs*)arg;
    char* path = args->path;
    const char* mode = args->mode;
    bool createParents = args->createParents;

    char* pathCopy = (char*)malloc(100 * sizeof(char));;
    strcpy(pathCopy, path);

    if(findRoute(pathCopy) != NULL) {
        printf("mkdir: cannot create directory '%s': file exists\n", path);
        return false;
    } else {
        Directory* currentDirectory;

        if(createParents) {
            // createParents 옵션 있을 경우
            if (path[0] == '/') {
                // 절대 경로인 경우 루트 디렉토리부터 시작
                currentDirectory = dirTree->root;
                path++;
            } else {
               // 상대 경로인 경우 현재 디렉토리부터 시작
                currentDirectory = dirTree->current;
            }
            Queue* queue = (Queue*)malloc(sizeof(Queue));
            initQueue(queue);

            char* token = strtok(pathCopy, "/");
            while (token != NULL) {
                enqueue(queue, token);
                token = strtok(NULL, "/");
            }

            char* dirName = (char*)malloc(100 * sizeof(char));
            while(true) {
                char* segment = dequeue(queue);
                if(isEmpty(queue)) {
                    strcpy(dirName, segment);
                    free(segment);
                    break;
                }

                if (strcmp(segment, "..") == 0) {
                    // 상위 디렉토리로 이동
                    if (currentDirectory->parent != NULL) {
                        currentDirectory = currentDirectory->parent;
                    }
                } else if (strcmp(segment, ".") == 0) {
                    // 현재 디렉토리 유지
                    continue;
                } else {
                    // 하위 디렉토리로 이동
                    Directory* tmp = currentDirectory->leftChild;
                    while (tmp != NULL && strcasecmp(tmp->name, segment) != 0) {
                        tmp = tmp->rightSibling;
                    }
                    if (tmp == NULL) {
                        // 해당 디렉토리가 없을 경우 : 옵션 켜져 있으므로 새로 만들어준다.
                        tmp = currentDirectory->leftChild;
                        if(tmp == NULL) {
                            strcpy(dirName, segment);
                            Directory* newDir = createNewDirectory(dirName, mode);
                            newDir->parent = currentDirectory;
                            addDirectoryRoute(newDir, currentDirectory->parent, dirName);
                            tmp->leftChild = newDir;
                            currentDirectory = newDir;
                        } else {
                            while(tmp->rightSibling != NULL) {
                                tmp = tmp->rightSibling;
                            }
                            strcpy(dirName, segment);
                            Directory* newDir = createNewDirectory(dirName, mode);
                            newDir->parent = currentDirectory;
                            addDirectoryRoute(newDir, currentDirectory->parent, dirName);
                            tmp->rightSibling = newDir;
                            currentDirectory = newDir;
                        }

                        Directory* newDir;
                        while(!isEmpty(queue)) {
                            segment = dequeue(queue);
                            strcpy(dirName, segment);
                            newDir = createNewDirectory(dirName, mode);
                            newDir->parent = currentDirectory;
                            currentDirectory->leftChild = newDir;
                            addDirectoryRoute(newDir, currentDirectory, dirName);
                            currentDirectory = newDir;
                        }
                        updateDirectoryFile();
                        return (void*)newDir;
                    }
                    currentDirectory = tmp;
                }
                free(segment);
            }
            Directory* newDir = createNewDirectory(dirName, mode);
            newDir->parent = currentDirectory;
            addDirectoryRoute(newDir, currentDirectory->parent, dirName);
            Directory* tmp = currentDirectory->leftChild;

            if(tmp == NULL) {
                currentDirectory->leftChild = newDir;
            } else {
                while(tmp->rightSibling != NULL) {
                    tmp = tmp->rightSibling;
                }
                tmp->rightSibling = newDir;
            }
        } else {
            // createParents 옵션 없을 경우
            if (path[0] == '/') {
                // 절대 경로인 경우 루트 디렉토리부터 시작
                currentDirectory = dirTree->root;
                path++;
            } else {
               // 상대 경로인 경우 현재 디렉토리부터 시작
                currentDirectory = dirTree->current;
            }
            Queue* queue = (Queue*)malloc(sizeof(Queue));
            initQueue(queue);

            char* token = strtok(pathCopy, "/");
            while (token != NULL) {
                enqueue(queue, token);
                token = strtok(NULL, "/");
            }
            
            char* dirName = (char*)malloc(100 * sizeof(char));
            while(true) {
                char* segment = dequeue(queue);
                if(isEmpty(queue)) {
                    strcpy(dirName, segment);
                    free(segment);
                    break;
                }
                
                if (strcmp(segment, "..") == 0) {
                    // 상위 디렉토리로 이동
                    if (currentDirectory->parent != NULL) {
                        currentDirectory = currentDirectory->parent;
                    }
                } else if (strcmp(segment, ".") == 0) {
                    // 현재 디렉토리 유지
                    continue;
                } else {
                    // 하위 디렉토리로 이동
                    Directory* tmp = currentDirectory->leftChild;
                    while (tmp != NULL && strcasecmp(tmp->name, segment) != 0) {
                        tmp = tmp->rightSibling;
                    }
                    if (tmp == NULL) {
                        // 해당 디렉토리가 없을 경우 : 옵션 꺼져 있으므로 에러낸다.
                        freeQueue(queue);
                        printf("mkdir: %s: No such file or directory.\n", path);
                        return NULL;
                    }
                    currentDirectory = tmp;
                }
                free(segment);
            }

            Directory* newDir = createNewDirectory(dirName, mode);
            newDir->parent = currentDirectory;
            addDirectoryRoute(newDir, currentDirectory->parent, dirName);
            Directory* tmp = currentDirectory->leftChild;

            if(tmp == NULL) {
                currentDirectory->leftChild = newDir;
            } else {
                while(tmp->rightSibling != NULL) {
                    tmp = tmp->rightSibling;
                }
                tmp->rightSibling = newDir;
            }
            returnDirectory = newDir;
        }
        updateDirectoryFile();
    }
    
    return (void*)returnDirectory;
    pthread_exit(NULL);
}