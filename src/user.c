#include "../header/header.h"

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