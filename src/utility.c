#include "../header/header.h"

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