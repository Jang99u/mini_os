#include "../header/header.h"

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