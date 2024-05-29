#include "../header/header.h"

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