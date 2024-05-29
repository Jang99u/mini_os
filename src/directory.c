#include "../header/header.h"

// 구조체를 기반으로 Directory.txt 파일에 파일 정보를 작성
void writeDirectoryToFile(FILE* file, Directory* directory) {
    if(directory == NULL) {
        return;
    }

    if(strcmp(directory->name, "/") == 0) {
        fprintf(file, "%s %c %d %d%d%d %d %d %d %d %d %d %d\n",
            directory->name,
            directory->type,
            directory->visible,
            (directory->permission[0] << 2) | (directory->permission[1] << 1) | directory->permission[2],
            (directory->permission[3] << 2) | (directory->permission[4] << 1) | directory->permission[5],
            (directory->permission[6] << 2) | (directory->permission[7] << 1) | directory->permission[8],
            directory->UID,
            directory->GID,
            directory->size,
            directory->month,
            directory->day,
            directory->hour,
            directory->minute
        );
    } else {
        fprintf(file, "%s %c %d %d%d%d %d %d %d %d %d %d %d %s\n",
            directory->name,
            directory->type,
            directory->visible,
            (directory->permission[0] << 2) | (directory->permission[1] << 1) | directory->permission[2],
            (directory->permission[3] << 2) | (directory->permission[4] << 1) | directory->permission[5],
            (directory->permission[6] << 2) | (directory->permission[7] << 1) | directory->permission[8],
            directory->UID,
            directory->GID,
            directory->size,
            directory->month,
            directory->day,
            directory->hour,
            directory->minute,
            directory->parent->route
        );
    }

    writeDirectoryToFile(file, directory->rightSibling);
    writeDirectoryToFile(file, directory->leftChild);
}

// 구조체를 기반으로 Directory.txt 파일에 파일 정보를 작성
void updateDirectoryFile() {
    FILE* file = fopen("../information/Directory.txt", "w");
    writeDirectoryToFile(file, dirTree->root);
    fclose(file);
}

// 디렉토리 구조체에 저장된 시간을 변경
void setDirectoryTime(Directory* directory) {
    time_t now;
    struct tm* currentTime;

    // 현재 시간 가져오기
    time(&now);
    currentTime = localtime(&now);

    // 디렉토리 구조체에 저장되어 있는 시간값 바꾸기
    directory->month = currentTime->tm_mon + 1;  
    directory->day = currentTime->tm_mday;
    directory->hour = currentTime->tm_hour;
    directory->minute = currentTime->tm_min;
}

// Print the month based on an integer value
void getMonth(int month) {
    switch(month){
    case 1:
        printf("Jan ");
        break;
    case 2:
        printf("Feb ");
        break;
    case 3:
        printf("Mar ");
        break;
    case 4:
        printf("Apr ");
        break;
    case 5:
        printf("May ");
        break;
    case 6:
        printf("Jun ");
        break;
    case 7:
        printf("Jul ");
        break;
    case 8:
        printf("Aug ");
        break;
    case 9:
        printf("Sep ");
        break;
    case 10:
        printf("Oct ");
        break;
    case 11:
        printf("Nov ");
        break;
    case 12:
        printf("Dec ");
        break;
    default:
        break;
    }
}

void buildDirectoryRoute(Queue* queue, Directory* parent, Directory* myAddress) {
    if(isEmpty(queue)) {
        myAddress->parent = parent;
        myAddress->leftChild = NULL;
        if(parent->leftChild == NULL) {
            parent->leftChild = myAddress;
            myAddress->rightSibling = NULL;
        } else {
            Directory* tmp = parent->leftChild;
            while(tmp->rightSibling != NULL) {
                tmp = tmp->rightSibling;
            }
            tmp->rightSibling = myAddress;
            myAddress->rightSibling = NULL;
        }
    } else {
        char* str = dequeue(queue);
        Directory* tmp = parent->leftChild;
        while(strcmp(tmp->name, str) != 0) {
            tmp = tmp->rightSibling;
            if(tmp == NULL) {
                // 찾고자 하는 경로가 없는 에러 상황
                printf("디렉토리 빌드 중 에러 발생\n");
                return;
            }
        }
        free(str);

        buildDirectoryRoute(queue, tmp, myAddress);
    }
}

// Directory.txt 내의 문자열을 바탕으로 Directory 구조체를 형성
void buildDirectoryNode(char* dirstr) {
    Directory* directory = (Directory*)malloc(sizeof(Directory));
    char name[MAX_ROUTE];
    char routestr[MAX_ROUTE];

    char* tmp = strtok(dirstr, " "); // name
    strncpy(directory->name, tmp, MAX_NAME);
    
    tmp = strtok(NULL, " "); // type
    directory->type = tmp[0];
    
    tmp = strtok(NULL, " "); // visible
    directory->visible = atoi(tmp);
    
    tmp = strtok(NULL, " "); // permission
    atoiPermission(directory, tmp);
    
    tmp = strtok(NULL, " "); // uid
    directory->UID = atoi(tmp);
    
    tmp = strtok(NULL, " "); // gid
    directory->GID = atoi(tmp);
    
    tmp = strtok(NULL, " "); // size
    directory->size = atoi(tmp);
    
    tmp = strtok(NULL, " "); // month
    directory->month = atoi(tmp);

    tmp = strtok(NULL, " "); // day
    directory->day = atoi(tmp);

    tmp = strtok(NULL, " "); // hour
    directory->hour = atoi(tmp);

    tmp = strtok(NULL, " "); // minute
    directory->minute = atoi(tmp);
    
    tmp = strtok(NULL, " "); // route
    strcpy(routestr, directory->name);
    
    if(tmp == NULL) {
        strncpy(directory->route, "/", MAX_ROUTE);
        directory->parent = NULL;
        directory->leftChild = NULL;
        directory->rightSibling = NULL;

        dirTree->root = directory;
    } else {
        if(strcmp(tmp, "/") == 0) {
            strcpy(name, tmp);
            strcat(name, routestr);
            strncpy(directory->route, name, MAX_ROUTE);
        } else {
            strcpy(name, tmp);
            strcat(name, "/");
            strcat(name, routestr);
            strncpy(directory->route, name, MAX_ROUTE);
        }

        Queue* queue = (Queue*)malloc(sizeof(Queue));
        initQueue(queue);
        char* route = strtok(tmp, "/");
        buildQueue(queue, route);
        buildDirectoryRoute(queue, dirTree->root, directory);

        freeQueue(queue);
    }
}

// Load the directory structure from a file
DirectoryTree* loadDirectory() {
    dirTree = (DirectoryTree*)malloc(sizeof(DirectoryTree));
    
    char tmp[MAX_LENGTH];
    
    FILE* directory = fopen("../information/Directory.txt", "r");
    while(fgets(tmp, MAX_LENGTH, directory) != NULL){
        tmp[strlen(tmp)-1] = '\0';
        buildDirectoryNode(tmp);
    }
    fclose(directory);
    dirTree->root->parent = dirTree->root;

    return dirTree;
}

// Recursively find a route to a directory
Directory* findRouteRecursive(Queue* queue, Directory* parent) {
    if(isEmpty(queue)) {
        return parent;
    } else {
        char* str = dequeue(queue);
        Directory* tmp = parent->leftChild;
        if(tmp == NULL) {
            printf("에러 발생\n");
            return NULL;
        }
        while(strcasecmp(tmp->name, str) != 0) {
            tmp = tmp->rightSibling;
            if(tmp == NULL) {
                // 찾고자 하는 경로가 없는 에러 상황
                printf("에러 발생\n");
                return NULL;
            }
        }
        free(str); 

        return findRouteRecursive(queue, tmp);
    }
}

// Find a route to a directory based on the given path
Directory* findRoute(char* pathOrigin) {
    char* path = (char*)malloc(100 * sizeof(char));;
    strcpy(path, pathOrigin);

    if (path == NULL || strcmp(path, "") == 0) {
        return dirTree->current;
    }

    Directory* currentDirectory;
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

    char* token = strtok(path, "/");
    while (token != NULL) {
        enqueue(queue, token);
        token = strtok(NULL, "/");
    }

    while (!isEmpty(queue)) {
        char* segment = dequeue(queue);

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
                freeQueue(queue);
                return NULL;
            }
            currentDirectory = tmp;
        }
        free(segment);
    }
    freeQueue(queue);
    return currentDirectory;
}