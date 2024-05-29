#include "../header/header.h"

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