#include "../header/header.h"

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