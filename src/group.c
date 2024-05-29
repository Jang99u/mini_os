#include "../header/header.h"

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