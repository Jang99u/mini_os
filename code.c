#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <utime.h>
#include <pthread.h>
#include <regex.h>

#define MAX_BUFFER 512
#define MAX_LENGTH 200
#define MAX_ROUTE 100
#define MAX_DIR 50
#define MAX_NAME 20
#define MAX_THREAD 50

#define DEFAULT printf("%c[%dm", 0x1B, 0)
#define BOLD printf("%c[%dm", 0x1B, 1)
#define WHITE printf("\x1b[37m")
#define BLUE printf("\x1b[34m")
#define GREEN printf("\x1b[32m")
#define RED printf("\x1b[31m")
#define RESET printf("\x1b[0m")

// Queue
#define MAX_QUEUE_SIZE 100
#define MAX_STRING_LENGTH 100

typedef struct {
    int front;
    int rear;
    int size;
    char* items[MAX_QUEUE_SIZE];
} Queue;

void initQueue(Queue* queue) {
    queue->front = 0;
    queue->rear = -1;
    queue->size = 0;
}

int isEmpty(Queue* queue) {
    return queue->size == 0;
}

int isFull(Queue* queue) {
    return queue->size == MAX_QUEUE_SIZE;
}

void enqueue(Queue* queue, const char* str) {
    if (isFull(queue)) {
        fprintf(stderr, "Queue overflow\n");
        return;
    }
    queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    queue->items[queue->rear] = strdup(str);
    queue->size++;
}

char* dequeue(Queue* queue) {
    if (isEmpty(queue)) {
        fprintf(stderr, "Queue underflow\n");
        return NULL;
    }
    char* str = queue->items[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    queue->size--;
    return str;
}

char* peek(Queue* queue) {
    if (isEmpty(queue)) {
        fprintf(stderr, "Queue is empty\n");
        return NULL;
    }
    return queue->items[queue->front];
}

void freeQueue(Queue* queue) {
    while (!isEmpty(queue)) {
        free(dequeue(queue));
    }
}

// Structure
// User
typedef struct UserNode {
    char name[MAX_NAME];
    char dir[MAX_DIR];
    int UID;
    int GID;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int sec;
    int wday; // 요일
} User;

typedef struct UserNodeList {
    struct UserNode* user;
    struct UserNodeList* nextUser;
} UserList;

// Group
typedef struct GroupNode {
    char name[MAX_NAME];
    int GID;
} Group;

typedef struct GroupNodeList {
    struct GroupNode* group;
    struct GroupNodeList* nextGroup;
} GroupList;

// Directory
typedef struct DirectoryNode {
    char name[MAX_NAME];
    char type;
    bool visible;
    bool permission[9];
    int UID;
	int GID;
    int size;
	int month;
    int day;
    int hour;
    int minute;
    char route[MAX_ROUTE];
	struct DirectoryNode* parent; // 상위 디렉토리
	struct DirectoryNode* leftChild; // 하위 디렉토리
    struct DirectoryNode* rightSibling; // 이웃 디렉토리
} Directory;

typedef struct DirectoryNodeTree {
	Directory* root;
    Directory* home;
	Directory* current;
} DirectoryTree;

// ls 스레드 구조
typedef struct {
    Directory* directory;
    bool showAll;
    bool showDetails;
} ListArgs;

typedef struct {
    char path[MAX_ROUTE];
    char mode[4];  // Permission string like "755"
    bool createParents;
} MkdirArgs;

typedef struct {
    char path[MAX_ROUTE];
    char mode[4];
} ChmodArgs;

typedef struct {
    bool showLineNumbers;
    bool ignoreCase;
    bool invertMatch;
    char* targetString;
    char* fileName;
} GrepArgs;

// Global Variable
DirectoryTree* dirTree;
User* loginUser;
UserList* userList;
GroupList* groupList;

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

// 문자열을 permission 배열로 변경
void atoiPermission(Directory* directory, char* perstr) {
    char userstr[2] = "";
    sprintf(userstr, "%c", perstr[0]);
    char groupstr[2] = "";
    sprintf(groupstr, "%c", perstr[1]);
    char otherstr[2] = "";
    sprintf(otherstr, "%c", perstr[2]);

    int permissionUser = atoi(userstr);
    int permissionGroup = atoi(groupstr);
    int permissionOther = atoi(otherstr);

    if(permissionUser >= 4) {
        directory->permission[0] = 1;
        permissionUser -= 4;
    } else directory->permission[0] = 0;
    if(permissionUser >= 2) {
        directory->permission[1] = 1;
        permissionUser -= 2;
    } else directory->permission[1] = 0;
    if(permissionUser >= 1) {
        directory->permission[2] = 1;
        permissionUser -= 1;
    } else directory->permission[2] = 0;

    if(permissionGroup >= 4) {
        directory->permission[3] = 1;
        permissionGroup -= 4;
    } else directory->permission[3] = 0;
    if(permissionGroup >= 2) {
        directory->permission[4] = 1;
        permissionGroup -= 2;
    } else directory->permission[4] = 0;
    if(permissionGroup >= 1) {
        directory->permission[5] = 1;
        permissionGroup -= 1;
    } else directory->permission[5] = 0;

    if(permissionOther >= 4) {
        directory->permission[6] = 1;
        permissionOther -= 4;
    } else directory->permission[6] = 0;
    if(permissionOther >= 2) {
        directory->permission[7] = 1;
        permissionOther -= 2;
    } else directory->permission[7] = 0;
    if(permissionOther >= 1) {
        directory->permission[8] = 1;
        permissionOther -= 1;
    } else directory->permission[8] = 0;    
}

// 주어진 문자열 값을 바탕으로 디렉토리의 permission 값을 수정
void setPermission(Directory* directory, const char* mode) {
    for (int i = 0; i < 3; i++) {
        if (mode[i] < '0' || mode[i] > '7') {
            fprintf(stderr, "chmod: invalid mode: %s\n", mode);
            return;
        }
    }

    int permissionUser = mode[0] - '0';
    int permissionGroup = mode[1] - '0';
    int permissionOther = mode[2] - '0';

    directory->permission[0] = permissionUser >= 4 ? 1 : 0; permissionUser %= 4;
    directory->permission[1] = permissionUser >= 2 ? 1 : 0; permissionUser %= 2;
    directory->permission[2] = permissionUser >= 1 ? 1 : 0;

    directory->permission[3] = permissionGroup >= 4 ? 1 : 0; permissionGroup %= 4;
    directory->permission[4] = permissionGroup >= 2 ? 1 : 0; permissionGroup %= 2;
    directory->permission[5] = permissionGroup >= 1 ? 1 : 0;

    directory->permission[6] = permissionOther >= 4 ? 1 : 0; permissionOther %= 4;
    directory->permission[7] = permissionOther >= 2 ? 1 : 0; permissionOther %= 2;
    directory->permission[8] = permissionOther >= 1 ? 1 : 0;
}

// 문자열을 '/' 문자를 기반으로 큐에 집어 넣어 큐를 형성
void buildQueue(Queue* queue, char* queuestr) {
    if(queuestr == NULL) {
        return;
    } else {
        enqueue(queue, queuestr);
        queuestr = strtok(NULL, "/");
        buildQueue(queue, queuestr);
    }
}

// 
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

// Find user by UID
char* findUserById(int UID) {
    UserList* currentUser = userList;
    while (currentUser != NULL) {
        if (currentUser->user->UID == UID) {
            return currentUser->user->name;
        }
        currentUser = currentUser->nextUser;
    }
    return NULL;
}

// Find group by GID
char* findGroupById(int GID) {
    GroupList* currentGroup = groupList;
    while (currentGroup != NULL) {
        if (currentGroup->group->GID == GID) {
            return currentGroup->group->name;
        }
        currentGroup = currentGroup->nextGroup;
    }
    return NULL;
}

// Count the number of links to a directory
int countLink(Directory* directory) {
    int link = 0;
    Directory* child = directory->leftChild;

    while(child != NULL) {
        link += 1;
        child = child->rightSibling;
    }

    if(directory->type == 'd') {
        link += 2;
    } else if(directory->type == '-') {
        link += 1;
    }

    return link;
}



// ls를 위한 메서드들

// Show directory details
void showDirectoryDetail(Directory* temp, char* name) {
    char permission[11] = {0};
    permission[0] = (temp->type == 'd') ? 'd' : '-';
    permission[1] = (temp->permission[0]) ? 'r' : '-';
    permission[2] = (temp->permission[1]) ? 'w' : '-';
    permission[3] = (temp->permission[2]) ? 'x' : '-';
    permission[4] = (temp->permission[3]) ? 'r' : '-';
    permission[5] = (temp->permission[4]) ? 'w' : '-';
    permission[6] = (temp->permission[5]) ? 'x' : '-';
    permission[7] = (temp->permission[6]) ? 'r' : '-';
    permission[8] = (temp->permission[7]) ? 'w' : '-';
    permission[9] = (temp->permission[8]) ? 'x' : '-';
    char* userName = findUserById(temp->UID);
    char* groupName = findGroupById(temp->GID);
    int link = countLink(temp);

    printf("%s %2d %-8s %-8s %5d ", 
        permission,
        link,
        userName,
        groupName,
        temp->size
    );

    getMonth(temp->month);
    printf("%02d %02d:%02d ",
        temp->day,
        temp->hour,
        temp->minute
    );

    if(temp->type == 'd') {
        BLUE;
        printf("%s\n", name);
        DEFAULT;
    } else if(temp->type == '-') {
        printf("%s\n", name);
    }
}

// List directories with options for showing all and detailed view
void listDirectory(Directory* directory, bool showAll, bool showDetails) {
    Directory* temp = directory->leftChild;

    if(temp == NULL && showAll == false) {
        // 디렉토리가 비어있는 경우 바로 반환
        return;
    }

    if(showAll && showDetails) {
        showDirectoryDetail(directory, ".");
        showDirectoryDetail(directory->parent, "..");
    } else if (showAll && !showDetails) {
        BLUE;
        printf("%-10s", ".");
        printf("%-10s", "..");
        DEFAULT;
    }

    while (temp != NULL) {
        if (temp->visible || showAll) {
            if (showDetails) {
                showDirectoryDetail(temp, temp->name);
            } else {
                if(temp->type == 'd') {
                    BLUE;
                    printf("%-10s", temp->name);
                    DEFAULT;
                } else if(temp->type == '-') {
                    printf("%-10s", temp->name);
                }
            }
        }
        temp = temp->rightSibling;
    }

    if (!showDetails) {
        printf("\n");
    }
}

// Thread function to list directories
void* listDirectoryThread(void* arg) {
    ListArgs* data = (ListArgs*)arg;
    Directory* directory = data->directory;
    bool showAll = data->showAll;
    bool showDetails = data->showDetails;

    printf("%s:\n", directory->name);
    listDirectory(directory, showAll, showDetails);
    printf("\n");

    free(data);
    pthread_exit(NULL);
}



// mkdir을 위한 메서드들 

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



// chmod를 위한 메서드들

// Thread function to change directory permissions
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



//cd를 위한 메서드들

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



// cat을 위한 메서드들

// 입력된 파일들을 기반으로 display
void catFiles(char* fileNames[], int fileCount) {
    for (int i = 0; i < fileCount; i++) {
        char filePath[256];
        snprintf(filePath, sizeof(filePath), "../file/%s", fileNames[i]);

        FILE* file = fopen(filePath, "r");
        if (file == NULL) {
            printf("cat: %s: No such file or directory\n", fileNames[i]);
            continue;
        } else if(findRoute(fileNames[i]) == NULL) {
            printf("cat: %s: No such file or directory\n", fileNames[i]);
            continue;
        }

        char line[MAX_BUFFER];
        while (fgets(line, sizeof(line), file) != NULL) {
            printf("%s", line);
        }
        fclose(file);
    }
}

// 입력된 파일들을 기반으로 display with number
void catFilesWithLineNumbers(char* fileNames[], int fileCount) {
    for (int i = 0; i < fileCount; i++) {
        int lineNumber = 1;
        char filePath[256];
        snprintf(filePath, sizeof(filePath), "../file/%s", fileNames[i]);
        
        FILE* file = fopen(filePath, "r");
        if (file == NULL) {
            printf("cat: %s: No such file or directory\n", fileNames[i]);
            continue;
        } else if(findRoute(fileNames[i]) == NULL) {
            printf("cat: %s: No such file or directory\n", fileNames[i]);
            continue;
        }

        char line[MAX_BUFFER];
        while (fgets(line, sizeof(line), file)) {
            printf("%6d\t%s", lineNumber++, line);
        }
        fclose(file);
    }
}

// Create a new file
void createFile(char* fileName) {
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "../file/%s", fileName);
    
    FILE* file = fopen(filePath, "w");
    if (file == NULL) {
        printf("cat: %s: Cannot create file\n", fileName);
        return;
    }

    char line[MAX_BUFFER];
    while (fgets(line, sizeof(line), stdin)) {
        fprintf(file, "%s", line);
    }
    rewind(stdin);

    fseek(file, 0, SEEK_END); // 파일 포인터를 파일 끝으로 이동
    long int size = ftell(file);
    char mode[4] = "644";
    pthread_t threads[MAX_THREAD];
    void* threadResult;

    MkdirArgs* args = (MkdirArgs*)malloc(sizeof(MkdirArgs));
    strncpy(args->path, fileName, MAX_ROUTE);
    strncpy(args->mode, mode, 4);
    args->createParents = false;

    pthread_create(&threads[0], NULL, makeDirectory, (void*)args);
    pthread_join(threads[0], &threadResult);

    Directory* newDir = (Directory*)threadResult;
    newDir->type = '-';
    newDir->size = size;
    updateDirectoryFile();

    fclose(file);
}

// 두 개의 파일을 묶어 하나의 파일로
void concatenateFilesToNewFile(char* inputFiles[], int inputFileCount, char* outputFile) {
    char outFilePath[256];
    snprintf(outFilePath, sizeof(outFilePath), "../file/%s", outputFile);

    FILE* outFile = fopen(outFilePath, "w");
    if (outFile == NULL) {
        printf("cat: %s: Cannot create file\n", outputFile);
        return;
    }

    for (int i = 0; i < inputFileCount; i++) {
        char inFilePath[256];
        snprintf(inFilePath, sizeof(inFilePath), "../file/%s", inputFiles[i]);

        FILE* inFile = fopen(inFilePath, "r");
        if(inFile == NULL) {
            printf("cat: %s: No such file or directory\n", inputFiles[i]);
            continue;
        } else if(findRoute(inputFiles[i]) == NULL) {
            printf("cat: %s: No such file or directory\n", inputFiles[i]);
            continue;
        }

        char line[MAX_BUFFER];
        while (fgets(line, sizeof(line), inFile) != NULL) {
            fprintf(outFile, "%s", line);
        }
        fclose(inFile);
    }
    
    fseek(outFile, 0, SEEK_END); // 파일 포인터를 파일 끝으로 이동
    long int size = ftell(outFile);
    char mode[4] = "644";
    pthread_t threads[MAX_THREAD];
    void* threadResult;

    MkdirArgs* args = (MkdirArgs*)malloc(sizeof(MkdirArgs));
    strncpy(args->path, outputFile, MAX_ROUTE);
    strncpy(args->mode, mode, 4);
    args->createParents = false;

    pthread_create(&threads[0], NULL, makeDirectory, (void*)args);
    pthread_join(threads[0], &threadResult);

    Directory* newDir = (Directory*)threadResult;
    newDir->type = '-';
    newDir->size = size;
    updateDirectoryFile();

    fclose(outFile);
}

// 파일 뒤에 내용 추가하기
void appendToFile(char* fileName) {
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "../file/%s", fileName);

    FILE* file = fopen(filePath, "a");
    if (file == NULL) {
        printf("cat: %s: Cannot open file\n", fileName);
        return;
    }

    bool isFileExist;
    if(findRoute(fileName) == NULL) {
        isFileExist = false;
    } else {
        isFileExist = true;
    }

    char line[MAX_BUFFER];
    while (fgets(line, sizeof(line), stdin)) {
        fprintf(file, "%s", line);
    }
    rewind(stdin);

    if(isFileExist == false) {
        fseek(file, 0, SEEK_END); // 파일 포인터를 파일 끝으로 이동
        long int size = ftell(file);
        char mode[4] = "644";
        pthread_t threads[MAX_THREAD];
        void* threadResult;

        MkdirArgs* args = (MkdirArgs*)malloc(sizeof(MkdirArgs));
        strncpy(args->path, fileName, MAX_ROUTE);
        strncpy(args->mode, mode, 4);
        args->createParents = false;

        pthread_create(&threads[0], NULL, makeDirectory, (void*)args);
        pthread_join(threads[0], &threadResult);

        Directory* newDir = (Directory*)threadResult;
        newDir->type = '-';
        newDir->size = size;
        updateDirectoryFile();
    } else {
        fseek(file, 0, SEEK_END); // 파일 포인터를 파일 끝으로 이동
        long int size = ftell(file);

        Directory* file_ = findRoute(fileName);
        file_->size = size;
        updateDirectoryFile();
    }

    fclose(file);
}



// touch를 위한 메서드들

// 파일 생성 및 시간 변경
void touchFile(char* fileName) {
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "../file/%s", fileName);
    
    // Update directory node time
    Directory* fileDir = findRoute(fileName);
    if (fileDir != NULL) {
        setDirectoryTime(fileDir);
    } else {
        FILE* file = fopen(filePath, "a");
        if (file == NULL) {
            printf("touch: %s: Cannot create file\n", fileName);
            return;
        }
        fclose(file);

        // Create a new file node in the directory structure
        char mode[4] = "644";
        pthread_t threads[MAX_THREAD];
        void* threadResult;

        MkdirArgs* args = (MkdirArgs*)malloc(sizeof(MkdirArgs));
        strncpy(args->path, fileName, MAX_ROUTE);
        strncpy(args->mode, mode, 4);
        args->createParents = false;

        pthread_create(&threads[0], NULL, makeDirectory, (void*)args);
        pthread_join(threads[0], &threadResult);

        Directory* newDir = (Directory*)threadResult;
        newDir->type = '-';
        newDir->size = 0;
    }
    updateDirectoryFile();
}


// 여러 파일의 시간 변경
void touchFiles(char* fileNames[], int fileCount) {
    for (int i = 0; i < fileCount; i++) {
        touchFile(fileNames[i]);
    }
}

// 특정 날짜시간 정보로 시간 변경
void touchFileWithTime(char* timeInfo, char* fileName) {
    struct tm newTime;
    strptime(timeInfo, "%Y%m%d%H%M", &newTime);
    time_t newTimestamp = mktime(&newTime);
    
    struct utimbuf newTimes;
    newTimes.actime = newTimestamp; // access time
    newTimes.modtime = newTimestamp; // modification time
    
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "../file/%s", fileName);
    
    if (utime(filePath, &newTimes) != 0) {
        perror("utime");
        return;
    }
    
    // Update directory node time
    Directory* fileDir = findRoute(fileName);
    if (fileDir != NULL) {
        fileDir->month = newTime.tm_mon + 1;
        fileDir->day = newTime.tm_mday;
        fileDir->hour = newTime.tm_hour;
        fileDir->minute = newTime.tm_min;
    }
    updateDirectoryFile();
}

// 현재 시간으로 시간 변경
void touchFileWithCurrentTime(char* fileName) {
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "../file/%s", fileName);
    
    Directory* fileDir = findRoute(fileName);
    if (fileDir != NULL) {
        setDirectoryTime(fileDir);
        updateDirectoryFile();
    }
}

// 다른 파일의 시간 정보를 복사
void touchFileWithReference(char* refFile, char* targetFile) {
    Directory* refDir = findRoute(refFile);
    if (refDir == NULL) {
        printf("touch: cannot stat '%s': No such file or directory\n", refFile);
        return;
    }
    
    Directory* targetDir = findRoute(targetFile);
    if (targetDir != NULL) {
        targetDir->month = refDir->month;
        targetDir->day = refDir->day;
        targetDir->hour = refDir->hour;
        targetDir->minute = refDir->minute;
    } else {
        touchFile(targetFile);
        targetDir = findRoute(targetFile);
        targetDir->month = refDir->month;
        targetDir->day = refDir->day;
        targetDir->hour = refDir->hour;
        targetDir->minute = refDir->minute;
    }
    updateDirectoryFile();
}



// cp를 위한 메서드드들

// 파일 복사 함수
void copyFile(Directory* source, Directory* destination) {
    // source 파일 경로 설정
    char sourcePath[256];
    snprintf(sourcePath, sizeof(sourcePath), "../file/%s", source->route);

    // destination 파일 경로 설정
    char destinationPath[256];
    snprintf(destinationPath, sizeof(destinationPath), "../file/%s", destination->route);

    FILE* sourceFile = fopen(sourcePath, "r");
    if (sourceFile == NULL) {
        printf("cp: cannot stat '%s': No such file or directory\n", sourcePath);
        return;
    }

    FILE* destinationFile = fopen(destinationPath, "w");
    if (destinationFile == NULL) {
        fclose(sourceFile);
        printf("cp: cannot create '%s': Permission denied\n", destinationPath);
        return;
    }

    char buffer[MAX_BUFFER];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0) {
        fwrite(buffer, 1, bytesRead, destinationFile);
    }

    fclose(sourceFile);
    fclose(destinationFile);

    // 디렉터리 구조 업데이트
    destination->size = source->size;
    destination->UID = source->UID;
    destination->GID = source->GID;
    setDirectoryTime(destination);
}


// 디렉토리 복사 함수
void copyDirectory(Directory* source, Directory* destinationParent, bool recursive, char* folderName) {
    Directory* newDir = createNewDirectory(folderName, "755");
    newDir->parent = destinationParent;
    addDirectoryRoute(newDir, destinationParent, folderName);

    if (destinationParent->leftChild == NULL) {
        destinationParent->leftChild = newDir;
    } else {
        Directory* sibling = destinationParent->leftChild;
        while (sibling->rightSibling != NULL) {
            sibling = sibling->rightSibling;
        }
        sibling->rightSibling = newDir;
    }

    if (recursive) {
        Directory* child = source->leftChild;
        while (child != NULL) {
            if (child->type == 'd') {
                copyDirectory(child, newDir, true, child->name);
            } else {
                Directory* newFile = createNewDirectory(child->name, "644");
                newFile->type = '-';
                newFile->size = child->size; 
                newFile->parent = newDir;
                addDirectoryRoute(newFile, newDir, child->name);

                if (newDir->leftChild == NULL) {
                    newDir->leftChild = newFile;
                } else {
                    Directory* fileSibling = newDir->leftChild;
                    while (fileSibling->rightSibling != NULL) {
                        fileSibling = fileSibling->rightSibling;
                    }
                    fileSibling->rightSibling = newFile;
                }
            }
            child = child->rightSibling;
        }
    }
}



// mv를 위한 메서드들

// 파일을 이동하는 메서드
void moveFile(Directory* source, Directory* destination, char* newName) {

    // 디렉토리 구조 업데이트
    Directory* newFile = createNewDirectory(newName, "644");
    newFile->type = '-';
    newFile->size = source->size;
    newFile->parent = destination;
    newFile->permission[0] = source->permission[0];
    newFile->permission[1] = source->permission[1];
    newFile->permission[2] = source->permission[2];
    newFile->permission[3] = source->permission[3];
    newFile->permission[4] = source->permission[4];
    newFile->permission[5] = source->permission[5];
    newFile->permission[6] = source->permission[6];
    newFile->permission[7] = source->permission[7];
    newFile->permission[8] = source->permission[8];
    addDirectoryRoute(source, destination, newName);

    if (destination->leftChild == NULL) {
        destination->leftChild = newFile;
    } else {
        Directory* sibling = destination->leftChild;
        while (sibling->rightSibling != NULL) {
            sibling = sibling->rightSibling;
        }
        sibling->rightSibling = newFile;
    }

    // 부모의 하위 디렉토리 리스트에서 삭제
    Directory* parent = source->parent;
    if (parent->leftChild == source) {
        parent->leftChild = source->rightSibling;
    } else {
        Directory* sibling = parent->leftChild;
        while (sibling->rightSibling != source) {
            sibling = sibling->rightSibling;
        }
        sibling->rightSibling = source->rightSibling;
    }

    updateDirectoryFile();
}

// 디렉토리를 이동하는 메서드
void moveDirectory(Directory* source, Directory* destination, char* newName, bool recursive) {

    // 디렉토리 구조 업데이트
    Directory* newDir = createNewDirectory(newName, "755");
    newDir->parent = destination;
    newDir->permission[0] = source->permission[0];
    newDir->permission[1] = source->permission[1];
    newDir->permission[2] = source->permission[2];
    newDir->permission[3] = source->permission[3];
    newDir->permission[4] = source->permission[4];
    newDir->permission[5] = source->permission[5];
    newDir->permission[6] = source->permission[6];
    newDir->permission[7] = source->permission[7];
    newDir->permission[8] = source->permission[8];
    addDirectoryRoute(newDir, destination, newName);

    if (destination->leftChild == NULL) {
        destination->leftChild = newDir;
    } else {
        Directory* sibling = destination->leftChild;
        while (sibling->rightSibling != NULL) {
            sibling = sibling->rightSibling;
        }
        sibling->rightSibling = newDir;
    }

    if (recursive) {
        Directory* child = source->leftChild;
        while (child != NULL) {
            if (child->type == 'd') {
                moveDirectory(child, newDir, child->name, true);
            } else {
                moveFile(child, newDir, child->name);
            }
            child = child->rightSibling;
        }
    }

    // 부모의 하위 디렉토리 리스트에서 삭제
    Directory* parent = source->parent;
    if (parent->leftChild == source) {
        parent->leftChild = source->rightSibling;
    } else {
        Directory* sibling = parent->leftChild;
        while (sibling->rightSibling != source) {
            sibling = sibling->rightSibling;
        }
        sibling->rightSibling = source->rightSibling;
    }

    updateDirectoryFile();
}



// grep을 위한 메서드


// 파일에서 문자열 검색 및 출력
void grepFile(GrepArgs* args) {
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "../file/%s", args->fileName);
    
    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        printf("grep: %s: No such file or directory\n", args->fileName);
        return;
    }

    char line[MAX_BUFFER];
    int lineNumber = 1;
    regex_t regex;
    int regexFlags = REG_EXTENDED | (args->ignoreCase ? REG_ICASE : 0);
    
    if (regcomp(&regex, args->targetString, regexFlags) != 0) {
        printf("grep: Invalid regular expression: %s\n", args->targetString);
        fclose(file);
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        bool matchFound = regexec(&regex, line, 0, NULL, 0) == 0;
        if ((matchFound && !args->invertMatch) || (!matchFound && args->invertMatch)) {
            if (args->showLineNumbers) {
                printf("%6d: ", lineNumber);
            }

            char* linePtr = line;
            char* matchPtr = strstr(linePtr, args->targetString);
            while (matchPtr != NULL) {
                int matchIndex = matchPtr - linePtr;
                printf("%.*s", matchIndex, linePtr);
                RED;
                printf("%s", args->targetString);
                RESET;
                linePtr = matchPtr + strlen(args->targetString);
                matchPtr = strstr(linePtr, args->targetString);
            }
            printf("%s", linePtr);
        }
        lineNumber++;
    }

    regfree(&regex);
    fclose(file);
}

// grep 명령어를 파싱하고 실행
void handleGrepCommand(char* command) {
    GrepArgs args = {false, false, false, NULL, NULL};
    char* saveptr;
    char* token = strtok_r(command, " ", &saveptr);
    
    while (token != NULL) {
        if (strcmp(token, "-n") == 0) {
            args.showLineNumbers = true;
        } else if (strcmp(token, "-i") == 0) {
            args.ignoreCase = true;
        } else if (strcmp(token, "-v") == 0) {
            args.invertMatch = true;
        } else if (args.targetString == NULL) {
            args.targetString = token;
        } else if (args.fileName == NULL) {
            args.fileName = token;
        }
        token = strtok_r(NULL, " ", &saveptr);
    }

    if (args.targetString == NULL || args.fileName == NULL) {
        printf("grep: missing operand\n");
        return;
    }

    grepFile(&args);
}


// useradd를 위한 메서드

// 새로운 사용자 정보를 User 구조체에 추가하고 User.txt 파일에 저장
User* addUser(char* username, int UID, int GID) {
    User* newUser = (User*)malloc(sizeof(User));
    strncpy(newUser->name, username, MAX_NAME);
    strncpy(newUser->dir, "/users/", MAX_DIR);
    strcat(newUser->dir, username);
    strcat(newUser->dir, "/");
    newUser->UID = UID;
    newUser->GID = GID;

    time_t now;
    struct tm* currentTime;
    time(&now);
    currentTime = localtime(&now);

    newUser->year = currentTime->tm_year + 1900;
    newUser->month = currentTime->tm_mon + 1;
    newUser->day = currentTime->tm_mday;
    newUser->hour = currentTime->tm_hour;
    newUser->minute = currentTime->tm_min;
    newUser->sec = currentTime->tm_sec;
    newUser->wday = currentTime->tm_wday;

    // 유저 리스트에 추가
    UserList* newUserList = (UserList*)malloc(sizeof(UserList));
    newUserList->user = newUser;
    newUserList->nextUser = userList;
    userList = newUserList;

    // User.txt 파일에 저장
    FILE* userFile = fopen("../information/User.txt", "a");
    fprintf(userFile, "%s %d %d %d %d %d %d %d %d %d %s\n",
        newUser->name,
        newUser->UID,
        newUser->GID,
        newUser->year,
        newUser->month,
        newUser->day,
        newUser->hour,
        newUser->minute,
        newUser->sec,
        newUser->wday,
        newUser->dir
    );
    fclose(userFile);

    return newUser;
}

// /users/ 폴더에 새로운 홈 디렉토리 생성
void createHomeDirectory(char* username, User* newUser) {
    char homeDir[MAX_ROUTE];
    snprintf(homeDir, sizeof(homeDir), "/users/%s", username);

    Directory* parentDir = findRoute("/users");
    if (parentDir == NULL) {
        printf("Error: /users directory does not exist.\n");
        return;
    }

    Directory* newDir = createNewDirectory(username, "755");
    newDir->parent = parentDir;
    newDir->UID = newUser->UID;
    newDir->GID = newUser->GID;

    if (parentDir->leftChild == NULL) {
        parentDir->leftChild = newDir;
    } else {
        Directory* sibling = parentDir->leftChild;
        while (sibling->rightSibling != NULL) {
            sibling = sibling->rightSibling;
        }
        sibling->rightSibling = newDir;
    }

    // 경로 설정
    snprintf(newDir->route, MAX_ROUTE, "/users/%s", username);
    
    // 디렉토리 파일 업데이트
    updateDirectoryFile();
}



// 입력된 명령어를 기반으로 메서드를 구분

// Classify and execute the given command
void classificationCommand(char* cmd) {
    if(strcmp(cmd, "") == 0 || cmd[0] == ' ') return;

    Directory* targetDirectory;
    char* saveptr;
    char* command = strtok_r(cmd, " ", &saveptr);

    if(strcmp(command, "ls") == 0) {
        command = strtok_r(NULL, " ", &saveptr);
        bool showAll = false;
        bool showDetails = false;

        if (command != NULL && command[0] == '-') {
            // 옵션이 있는 경우
            if (strcmp(command, "-a") == 0) {
                showAll = true;
            } else if (strcmp(command, "-l") == 0) {
                showDetails = true;
            } else if (strcmp(command, "-al") == 0 || strcmp(command, "-la") == 0) {
                showAll = true;
                showDetails = true;
            } else {
                char* error = strtok_r(command, "-", &saveptr);
                printf("ls: invalid option -- '%s'\n", error);
                return;
            }
            command = strtok_r(NULL, " ", &saveptr);
        }

        pthread_t threads[MAX_THREAD];
        int threadCount = 0;

        if(command == NULL) {
            targetDirectory = dirTree->current;
            listDirectory(targetDirectory, showAll, showDetails); // 타겟 디렉토리의 내용을 나열
        } else {
            while(command != NULL) {
                targetDirectory = findRoute(command);

                if (targetDirectory == NULL) {
                    printf("ls: No such file or directory: %s\n", command);
                } else {
                    ListArgs* data = (ListArgs*)malloc(sizeof(ListArgs));
                    data->directory = targetDirectory;
                    data->showAll = showAll;
                    data->showDetails = showDetails;

                    pthread_create(&threads[threadCount], NULL, listDirectoryThread, (void*)data);
                    pthread_join(threads[threadCount], NULL);
                    threadCount++;
                }
                command = strtok_r(NULL, " ", &saveptr);
            }
        }

    } else if(strcmp(command, "cd") == 0) {
        command = strtok_r(NULL, " ", &saveptr);
        changeDirectory(command);

    } else if(strcmp(command, "mkdir") == 0) {
        char mode[4] = "755";
        bool createParents = false;

        command = strtok_r(NULL, " ", &saveptr);
        while (command != NULL && command[0] == '-') {
            // 옵션이 있는 경우
            if (strcmp(command, "-m") == 0) {
                command = strtok_r(NULL, " ", &saveptr);
                strncpy(mode, command, 4);
            } else if (strcmp(command, "-p") == 0) {
                createParents = true;
            }
            command = strtok_r(NULL, " ", &saveptr);
        }

        pthread_t threads[MAX_THREAD];
        int threadCount = 0;

        while (command != NULL) {
            MkdirArgs* args = (MkdirArgs*)malloc(sizeof(MkdirArgs));
            strncpy(args->path, command, MAX_ROUTE);
            strncpy(args->mode, mode, 4);
            args->createParents = createParents;

            pthread_create(&threads[threadCount], NULL, makeDirectory, (void*)args);
            pthread_join(threads[threadCount], NULL); 
            threadCount++;

            command = strtok_r(NULL, " ", &saveptr);
        }
        
    } else if(strcmp(command, "cat") == 0) {
        command = strtok_r(NULL, " ", &saveptr);
        if (command == NULL) {
            printf("cat: missing operand\n");
            return;
        }

        if (strcmp(command, ">") == 0) {
            // cat > [파일명]
            command = strtok_r(NULL, " ", &saveptr);
            if (command == NULL) {
                printf("cat: missing file operand after '>'\n");
                return;
            }
            createFile(command);
        } else if (strcmp(command, ">>") == 0) {
            // cat >> [파일명]
            command = strtok_r(NULL, " ", &saveptr);
            if (command == NULL) {
                printf("cat: missing file operand after '>>'\n");
                return;
            }
            appendToFile(command);
        } else {
            bool showLineNumbers = false;
            if (strcmp(command, "-n") == 0) {
                showLineNumbers = true;
                command = strtok_r(NULL, " ", &saveptr);
            }

            char* files[MAX_BUFFER];
            int fileCount = 0;
            while (command != NULL && strcmp(command, ">>") != 0) {
                files[fileCount++] = command;
                command = strtok_r(NULL, " ", &saveptr);
            }

            if (command == NULL) {
                // cat [파일명] or cat -n [파일명]
                if (showLineNumbers) {
                    catFilesWithLineNumbers(files, fileCount);
                } else {
                    catFiles(files, fileCount);
                }
            } else {
                // cat [파일명] [파일명] >> [파일명]
                command = strtok_r(NULL, " ", &saveptr);
                if (command == NULL) {
                    printf("cat: missing file operand after '>>'\n");
                    return;
                }
                concatenateFilesToNewFile(files, fileCount, command);
            }
        }

    } else if(strcmp(command, "chmod") == 0) {
        command = strtok_r(NULL, " ", &saveptr);
        if (command == NULL) {
            printf("chmod: missing operand\n");
            return;
        }
        char mode[4];
        strncpy(mode, command, 4);
        command = strtok_r(NULL, " ", &saveptr);
        if (command == NULL) {
            printf("chmod: missing operand after '%s'\n", mode);
            return;
        }

        pthread_t threads[MAX_THREAD];
        int threadCount = 0;

        while (command != NULL) {
            ChmodArgs* args = (ChmodArgs*)malloc(sizeof(ChmodArgs));
            strncpy(args->path, command, MAX_ROUTE);
            strncpy(args->mode, mode, 4);

            pthread_create(&threads[threadCount], NULL, changeMode, (void*)args);
            threadCount++;

            command = strtok_r(NULL, " ", &saveptr);
        }

        for (int i = 0; i < threadCount; i++) {
            pthread_join(threads[i], NULL);
        }

        updateDirectoryFile(); // 파일에 변경된 내용 반영

    } else if(strcmp(command, "grep") == 0) {
        handleGrepCommand(saveptr);

    }  else if (strcmp(command, "cp") == 0) {
        bool recursive = false;
        char* option = strtok_r(NULL, " ", &saveptr);

        // 옵션이 있는지 확인
        if (option != NULL && strcmp(option, "-r") == 0) {
            recursive = true;
            option = strtok_r(NULL, " ", &saveptr);
        }

        char* sourcePath = option;
        char* destinationPath = strtok_r(NULL, " ", &saveptr);

        if (sourcePath == NULL || destinationPath == NULL) {
            printf("cp: missing file operand\n");
            return;
        }

        Directory* source = findRoute(sourcePath);
        Directory* destination = findRoute(destinationPath);

        if (source == NULL) {
            printf("cp: cannot stat '%s': No such file or directory\n", sourcePath);
            return;
        }

        if (destination != NULL && source->type == 'd') {
            // 복사하고자 하는 것이 디렉토리인 경우
            if (recursive == false) {
                printf("cp: %s is a directory (not copied).\n", source->name);
                return;
            } else {
                if (destination->type == 'd') {
                    copyDirectory(source, destination, recursive, source->name);
                } else {
                    printf("cp: cannot overwrite non-directory '%s' with directory '%s'\n", destinationPath, sourcePath);
                }
            } 
        } else if (destination != NULL && source->type == '-') {
            // 복사하고자 하는 것이 파일인 경우
            Directory* newFile = createNewDirectory(source->name, "644");
            newFile->type = '-';
            newFile->size = source -> size;
            newFile->parent = destination;
            addDirectoryRoute(newFile, destination, source->name);

            if (destination->leftChild == NULL) {
                destination->leftChild = newFile;
            } else {
                Directory* sibling = destination->leftChild;
                while (sibling->rightSibling != NULL) {
                    sibling = sibling->rightSibling;
                }
                sibling->rightSibling = newFile;
            }
        } else if (destination == NULL) {
            // 목적지 경로가 디렉토리가 아닌 경우
            char* destName = strrchr(destinationPath, '/');
            if (destName != NULL) {
                *destName = '\0';
                destination = findRoute(destinationPath);
                *destName = '/';
                destName++;
            } else {
                destination = dirTree->current;
                destName = destinationPath;
            }

            if (destination == NULL || destination->type != 'd') {
                printf("cp: cannot create '%s': No such directory\n", destinationPath);
                return;
            }

            if (source->type == '-') {
                Directory* newFile = createNewDirectory(destName, "644");
                newFile->type = '-';
                newFile->size = source->size;
                newFile->parent = destination;
                addDirectoryRoute(newFile, destination, destName);
                if(strcmp(source->name, newFile->name) != 0) {
                    copyFile(source, newFile);
                }

                if (destination->leftChild == NULL) {
                    destination->leftChild = newFile;
                } else {
                    Directory* sibling = destination->leftChild;
                    while (sibling->rightSibling != NULL) {
                        sibling = sibling->rightSibling;
                    }
                    sibling->rightSibling = newFile;
                }
            } else if (source->type == 'd') {
                if (recursive == false) {
                    printf("cp: %s is a directory (not copied).\n", source->name);
                    return;
                } else {
                    if (destination->type == 'd') {
                        copyDirectory(source, destination, recursive, destName);
                    } else {
                        printf("cp: cannot overwrite non-directory '%s' with directory '%s'\n", destinationPath, sourcePath);
                    }
                } 
            }
        }
        updateDirectoryFile();
        
    } else if(strcmp(command, "touch") == 0) {
        char* option = strtok_r(NULL, " ", &saveptr);
        
        if(option == NULL) {
            printf("touch: missing file operand\n");
            return;
        }
        
        if(strcmp(option, "-t") == 0) {
            char* timeInfo = strtok_r(NULL, " ", &saveptr);
            if (timeInfo == NULL) {
                printf("touch: missing time operand after '-t'\n");
                return;
            }
            option = strtok_r(NULL, " ", &saveptr);
            if (option == NULL) {
                printf("touch: missing file operand after time information\n");
                return;
            }
            touchFileWithTime(timeInfo, option);
        } else if(strcmp(option, "-c") == 0) {
            option = strtok_r(NULL, " ", &saveptr);
            if (option == NULL) {
                printf("touch: missing file operand after '-c'\n");
                return;
            }
            touchFileWithCurrentTime(option);
        } else if(strcmp(option, "-r") == 0) {
            char* refFile = strtok_r(NULL, " ", &saveptr);
            if (refFile == NULL) {
                printf("touch: missing reference file operand after '-r'\n");
                return;
            }
            option = strtok_r(NULL, " ", &saveptr);
            if (option == NULL) {
                printf("touch: missing file operand after reference file\n");
                return;
            }
            touchFileWithReference(refFile, option);
        } else {
            char* files[MAX_BUFFER];
            int fileCount = 0;
            while (option != NULL) {
                files[fileCount++] = option;
                option = strtok_r(NULL, " ", &saveptr);
            }
            touchFiles(files, fileCount);
        }

    } else if(strcmp(command, "pwd") == 0) {
        printf("%s\n", dirTree->current->route);

    } else if(strcmp(command, "mv") == 0) {
        bool recursive = false;
        char* option = strtok_r(NULL, " ", &saveptr);

        // 옵션이 있는지 확인
        if (option != NULL && strcmp(option, "-r") == 0) {
            recursive = true;
            option = strtok_r(NULL, " ", &saveptr);
        }

        char* sourcePath = option;
        char* destinationPath = strtok_r(NULL, " ", &saveptr);

        if (sourcePath == NULL || destinationPath == NULL) {
            printf("mv: missing file operand\n");
            return;
        }

        Directory* source = findRoute(sourcePath);
        Directory* destination = findRoute(destinationPath);

        if (source == NULL) {
            printf("mv: cannot stat '%s': No such file or directory\n", sourcePath);
            return;
        }

        if (destination != NULL && destination->type == 'd') {
            // 이동할 목적지가 디렉토리인 경우
            if (source->type == 'd' && recursive) {
                moveDirectory(source, destination, source->name, recursive);
            } else if (source->type == '-') {
                moveFile(source, destination, source->name);
            } else {
                printf("mv: omitting directory '%s'\n", source->name);
            }
        } else {
            // 이름을 바꾸는 경우
            char* newName = destinationPath;

            if (source->type == '-') {
                char sourcePath[256];
                snprintf(sourcePath, sizeof(sourcePath), "../file/%s", source->name);

                char destinationPath_[256];
                snprintf(destinationPath_, sizeof(destinationPath_), "../file/%s", newName);

                strcpy(source->name, newName);
                rename(sourcePath, destinationPath_);
                updateDirectoryFile();
            }
        }

    } else if(strcmp(command, "useradd") == 0) {
        if(loginUser->UID != 0) {
            printf("not permission\n");
            return;
        }

        char* username = strtok_r(NULL, " ", &saveptr);
        int UID = -1;
        int GID = -1;

        if (username == NULL) {
            printf("useradd: missing operand\n");
            return;
        }

        command = strtok_r(NULL, " ", &saveptr);
        while (command != NULL) {
            if (strcmp(command, "-u") == 0) {
                command = strtok_r(NULL, " ", &saveptr);
                if (command == NULL) {
                    printf("useradd: missing UID operand after '-u'\n");
                    return;
                }
                UID = atoi(command);
            } else if (strcmp(command, "-g") == 0) {
                command = strtok_r(NULL, " ", &saveptr);
                if (command == NULL) {
                    printf("useradd: missing GID operand after '-g'\n");
                    return;
                }
                GID = atoi(command);
            }
            command = strtok_r(NULL, " ", &saveptr);
        }

        if (UID == -1 || GID == -1) {
            printf("useradd: missing mandatory option(s)\n");
            return;
        }

        if (findGroupById(GID) == NULL) {
            printf("not exist group\n");
            return;
        }

        User* newUser = addUser(username, UID, GID);
        createHomeDirectory(username, newUser);
        printf("User '%s' added with home directory '/users/%s'\n", username, username);

    } else if(strcmp(command, "exit") == 0) {
        printf("%s", "Logout\n");
        exit(0);
    } else {
        printf("command not found: %s\n", command);
        return;
    }
}



// 헤더 부분을 출력
void printHeader(DirectoryTree* workingDirectory, User* user) {
    BOLD;GREEN;
    printf("%s@1-os-linux", user->name);
    DEFAULT;

    printf(":");

    BOLD;BLUE;
    printf("%s", workingDirectory->current->route);
    DEFAULT;

    printf("#");
}

// 유저 정보를 로드 
void loadUser() {
    FILE* userFile = fopen("../information/User.txt", "r");
    userList = (UserList*)malloc(sizeof(UserList));
    char tmp[MAX_LENGTH]; 

    while(fgets(tmp, MAX_LENGTH, userFile) != NULL){
        User* newUser = (User*)malloc(sizeof(User));

        tmp[strlen(tmp)-1] = '\0';

        char* ptr = strtok(tmp, " "); // name
        strncpy(newUser->name, ptr, MAX_NAME);

        ptr = strtok(NULL, " "); // UID
        newUser->UID = atoi(ptr);

        ptr = strtok(NULL, " "); // GID
        newUser->GID = atoi(ptr);

        ptr = strtok(NULL, " "); // year
        newUser->year = atoi(ptr);

        ptr = strtok(NULL, " "); // month
        newUser->month = atoi(ptr);

        ptr = strtok(NULL, " "); // day
        newUser->day = atoi(ptr);

        ptr = strtok(NULL, " "); // hour
        newUser->hour = atoi(ptr);

        ptr = strtok(NULL, " "); // minute
        newUser->minute = atoi(ptr);
        
        ptr = strtok(NULL, " "); // sec
        newUser->sec = atoi(ptr);

        ptr = strtok(NULL, " "); // wday
        newUser->wday = atoi(ptr);
        
        ptr = strtok(NULL, " ");
        strncpy(newUser->dir, ptr, MAX_NAME);
        
        if(strcmp(newUser->name, "root") == 0) {
            userList->user = newUser;
            userList->nextUser = NULL;
        } else {
            UserList* tmpRoute = userList;
            while(tmpRoute->nextUser != NULL) {
                tmpRoute = tmpRoute->nextUser;
            }
            UserList* nextUserList = (UserList*)malloc(sizeof(UserList));
            nextUserList->user = newUser;
            nextUserList->nextUser = NULL;

            tmpRoute->nextUser = nextUserList;
        }   
    }
    fclose(userFile);
    return;
}

// 그룹 정보를 로드
void loadGroup() {
    FILE* groupFile = fopen("../information/Group.txt", "r");
    groupList = (GroupList*)malloc(sizeof(GroupList));
    char tmp[MAX_LENGTH]; 

    while(fgets(tmp, MAX_LENGTH, groupFile) != NULL){
        Group* newGroup = (Group*)malloc(sizeof(Group));

        tmp[strlen(tmp)-1] = '\0';

        char* ptr = strtok(tmp, " "); // name
        strncpy(newGroup->name, ptr, MAX_NAME);

        ptr = strtok(NULL, " "); // GID
        newGroup->GID = atoi(ptr);
        
        if(strcmp(newGroup->name, "wheel") == 0) {
            groupList->group = newGroup;
            groupList->nextGroup = NULL;
        } else {
            GroupList* tmpRoute = groupList;
            while(tmpRoute->nextGroup != NULL) {
                tmpRoute = tmpRoute->nextGroup;
            }
            GroupList* nextGroupList = (GroupList*)malloc(sizeof(GroupList));
            nextGroupList->group = newGroup;
            nextGroupList->nextGroup = NULL;

            tmpRoute->nextGroup = nextGroupList;
        }   
    }
    fclose(groupFile);
    return;
}

User* login() {
    User* user;
    UserList* users = userList;
    char name[MAX_NAME];
    bool isExist = false;

    printf("Users: ");
    while(users != NULL) {
        printf("%s ", users->user->name);
        users = users->nextUser;
    }
    printf("\n");

    while(isExist != true) {
        printf("Login: ");
        fgets(name, sizeof(name), stdin);
        name[strlen(name)-1] = '\0';

        users = userList;
        while(users != NULL) {
            if(strcmp(name, users->user->name) == 0) {
                user = users->user;
                isExist = true;
                break;
            }
            users = users->nextUser;
        }
        if(isExist == false) {
            printf("'%s' User does not exists\n", name);    
        }
    }

    return user;
}

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