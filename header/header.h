#ifndef HEADER_H_
#define HEADER_H_

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
#define MAX_QUEUE_SIZE 100
#define MAX_STRING_LENGTH 100

#define DEFAULT printf("%c[%dm", 0x1B, 0)
#define BOLD printf("%c[%dm", 0x1B, 1)
#define WHITE printf("\x1b[37m")
#define BLUE printf("\x1b[34m")
#define GREEN printf("\x1b[32m")
#define RED printf("\x1b[31m")
#define RESET printf("\x1b[0m")

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

// 스레드 구조
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

// Queue
typedef struct {
    int front;
    int rear;
    int size;
    char* items[MAX_QUEUE_SIZE];
} Queue;

// Queue 관리 코드
// queue.c
void initQueue(Queue* queue);
int isEmpty(Queue* queue);
int isFull(Queue* queue);
void enqueue(Queue* queue, const char* str);
char* dequeue(Queue* queue);
char* peek(Queue* queue);
void freeQueue(Queue* queue);
void buildQueue(Queue* queue, char* queuestr);

// 유저 관리 코드
// user.c
char* findUserById(int UID);
void loadUser();

// 그룹 관리 코드
// group.c
char* findGroupById(int GID);char* findGroupById(int GID);
void loadGroup();

// 디렉토리 관리 코드
// directory.c
void writeDirectoryToFile(FILE* file, Directory* directory);
void updateDirectoryFile();
void setDirectoryTime(Directory* directory);
void getMonth(int month);
void buildDirectoryRoute(Queue* queue, Directory* parent, Directory* myAddress);
void buildDirectoryNode(char* dirstr);
DirectoryTree* loadDirectory();
Directory* findRouteRecursive(Queue* queue, Directory* parent);
Directory* findRoute(char* pathOrigin);

// 파일 권환 관리 코드
// permission.c
void atoiPermission(Directory* directory, char* perstr);
void setPermission(Directory* directory, const char* mode);

// 기타 utility 관련 코드
// utility.c
int countLink(Directory* directory);
void classificationCommand(char* cmd);
void printHeader(DirectoryTree* workingDirectory, User* user);
User* login();

// ls
// ls.c
void showDirectoryDetail(Directory* temp, char* name);
void listDirectory(Directory* directory, bool showAll, bool showDetails);
void* listDirectoryThread(void* arg);

// mkdir
// mkdir.c
Directory* createNewDirectory(char* name, const char* mode);
void addDirectoryRoute(Directory* newDir, Directory* parent, char* dirName);
void* makeDirectory(void* arg);

// chmod
// chmod.c
void* changeMode(void* arg);

// cd
// cd.c
void changeDirectory(char* path);

// cat
// cat.c
void catFiles(char* fileNames[], int fileCount);
void catFilesWithLineNumbers(char* fileNames[], int fileCount);
void createFile(char* fileName);
void concatenateFilesToNewFile(char* inputFiles[], int inputFileCount, char* outputFile);
void appendToFile(char* fileName);

// touch
// touch.c
void touchFile(char* fileName);
void touchFiles(char* fileNames[], int fileCount);
void touchFileWithTime(char* timeInfo, char* fileName);
void touchFileWithCurrentTime(char* fileName);
void touchFileWithReference(char* refFile, char* targetFile);

// cp
// cp.c
void copyFile(Directory* source, Directory* destination);
void copyDirectory(Directory* source, Directory* destinationParent, bool recursive, char* folderName);

// mv
// mv.c
void moveFile(Directory* source, Directory* destination, char* newName);
void moveDirectory(Directory* source, Directory* destination, char* newName, bool recursive);

// grep
// grep.c
void grepFile(GrepArgs* args);
void handleGrepCommand(char* command);

// useradd
// useradd.c
User* addUser(char* username, int UID, int GID);
void createHomeDirectory(char* username, User* newUser);

#endif