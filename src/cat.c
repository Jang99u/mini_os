#include "../header/header.h"

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