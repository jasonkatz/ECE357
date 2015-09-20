#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

void listDir(char * filePath);
void printData(char * dirPath, struct dirent * de);

int main(int argc, char **argv) {

    int opt;
    while ((opt = getopt(argc, argv, "u:m:")) != -1) {
        switch (opt) {
            case 'u':
                printf("Handle %c option for %s\n", opt, optarg);
                break;
            case 'm':
                printf("Handle %c option for %s\n", opt, optarg);
                break;
            default:
                exit(1);
        }
    }

    char * startingPath = argv[argc - 1];

    listDir(startingPath);

    return 0;
}

void listDir(char * dirPath) {
    if (dirPath[strlen(dirPath) - 1] != '/') {
        strcat(dirPath, "/");
    }

    printf("Listing: %s\n", dirPath);

    DIR * dirp = opendir(dirPath);
    if (!dirp) {
        printf("Cannot open directory %s:%s\n", dirPath, strerror(errno));
    }
    struct dirent * de;

    // Keep track of directories to explore
    char ** dirArray = (char **)malloc(sizeof(char *));
    int dirCount = 0;
    int dirArraySize = 1;

    while (de = readdir(dirp)) {
        // Skip hidden files or directories
        if (de->d_name[0] == '.') {
            continue;
        }

        // Add directories to dirArray
        if (de->d_type == DT_DIR) {
            dirArray[dirCount++] = de->d_name;
            if (dirCount == dirArraySize) {
                dirArraySize *= 2;
                dirArray = (char **)realloc(dirArray, dirArraySize * sizeof(char *));
            }
        }

        printData(dirPath, de);
    }

    // Loop through dirArray and listDir for each one
    int i = 0;
    while (i < dirCount) {
        char * tempDirPath = strdup(dirPath);
        char * tempPath = malloc(sizeof (char) * (strlen(tempDirPath) + strlen(dirArray[i]) + 2));
        tempPath = strcat(tempDirPath, dirArray[i]);
        //printf("Recursively listing %s\n", dirArray[i]);
        listDir(tempPath);
        //free(tempPath);

        ++i;
    }

    closedir(dirp);

    // Free dirArray
    //free(dirArray);
    dirArray = NULL;
    dirCount = 0;
}

void printData(char * dirPath, struct dirent * de) {
    char * path = malloc(strlen(dirPath) + strlen(de->d_name) + 1);
    strcpy(path, dirPath);
    strcat(path, de->d_name);

    struct stat sb;
    stat(dirPath, &sb);
    printf("\t%s\n", de->d_name);
    //printf("\t%s\n", path);
}
