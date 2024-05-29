#include "../header/header.h"

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