#include "../header/header.h"

// 파일 복사 함수
void copyFile(Directory* source, Directory* destination) {
    // source 파일 경로 설정
    char sourcePath[256];
    snprintf(sourcePath, sizeof(sourcePath), "../file/%s", source->route);

    // destination 파일 경로 설정
    char destinationPath[256];
    snprintf(destinationPath, sizeof(destinationPath), "../file/%s", destination->route);

    FILE* sourceFile = fopen(sourcePath, "r");
    if (sourceFile == NULL) {
        printf("cp: cannot stat '%s': No such file or directory\n", sourcePath);
        return;
    }

    FILE* destinationFile = fopen(destinationPath, "w");
    if (destinationFile == NULL) {
        fclose(sourceFile);
        printf("cp: cannot create '%s': Permission denied\n", destinationPath);
        return;
    }

    char buffer[MAX_BUFFER];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0) {
        fwrite(buffer, 1, bytesRead, destinationFile);
    }

    fclose(sourceFile);
    fclose(destinationFile);

    // 디렉터리 구조 업데이트
    destination->size = source->size;
    destination->UID = source->UID;
    destination->GID = source->GID;
    setDirectoryTime(destination);
}


// 디렉토리 복사 함수
void copyDirectory(Directory* source, Directory* destinationParent, bool recursive, char* folderName) {
    Directory* newDir = createNewDirectory(folderName, "755");
    newDir->parent = destinationParent;
    addDirectoryRoute(newDir, destinationParent, folderName);

    if (destinationParent->leftChild == NULL) {
        destinationParent->leftChild = newDir;
    } else {
        Directory* sibling = destinationParent->leftChild;
        while (sibling->rightSibling != NULL) {
            sibling = sibling->rightSibling;
        }
        sibling->rightSibling = newDir;
    }

    if (recursive) {
        Directory* child = source->leftChild;
        while (child != NULL) {
            if (child->type == 'd') {
                copyDirectory(child, newDir, true, child->name);
            } else {
                Directory* newFile = createNewDirectory(child->name, "644");
                newFile->type = '-';
                newFile->size = child->size; 
                newFile->parent = newDir;
                addDirectoryRoute(newFile, newDir, child->name);

                if (newDir->leftChild == NULL) {
                    newDir->leftChild = newFile;
                } else {
                    Directory* fileSibling = newDir->leftChild;
                    while (fileSibling->rightSibling != NULL) {
                        fileSibling = fileSibling->rightSibling;
                    }
                    fileSibling->rightSibling = newFile;
                }
            }
            child = child->rightSibling;
        }
    }
}