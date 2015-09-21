#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void listDir(char * filePath);
void printData(char * dirPath, struct dirent * de);
char getType(struct stat sb);
char * getPermissions(struct stat sb);
char * getUser(struct stat sb);
char * getGroup(struct stat sb);

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
    if (lstat(path, &sb) == -1) {
        printf("stat failed: %s\n", strerror(errno));
        exit(1);
    }

    // Print info appropriately
    printf("%04X/%d\t", (int)sb.st_dev, (int)sb.st_ino);
    printf("%c%s\t", getType(sb), getPermissions(sb));
    printf("%d\t", sb.st_nlink);
    printf("%s\t", getUser(sb));
    printf("%s\t", getGroup(sb));

    // Print size or raw device number
    if (getType(sb) == 'm' || getType(sb) == 'M') {
        printf("%X\t", (int)sb.st_rdev);
    } else {
        printf("%d\t", sb.st_size);
    }

    // Print last modification time
    struct tm * t = localtime(&(sb.st_mtim.tv_sec));
    printf("%04d-%02d-%02d %02d:%02d:%02d\t", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    printf("%s", path);
    printf("\n");
}

char getType(struct stat sb) {
    char c;

    switch(sb.st_mode & S_IFMT) {
    case S_IFREG:
        c = '-';
        break;
    case S_IFDIR:
        c = 'd';
        break;
    case S_IFLNK:
        c = 'l';
        break;
    case S_IFCHR:
        c = 'c';
        break;
    case S_IFBLK:
        c = 'b';
        break;
    case S_IFIFO:
        c = 'p';
        break;
    case S_IFSOCK:
        c = 's';
        break;
    default:
        // Other cases without macros
        switch((sb.st_mode & S_IFMT) >> 12) {
        case 3: // S_IFMPC
            c = 'm';
            break;
        case 5: //S_IFNAM
            c = 'n';
            break;
        case 7: //S_IFMPB
            c = 'M';
            break;
        case 9: //S_IFCMP
            c = 'C';
            break;
        case 11: //S_IFSHAD
            c = 's';
            break;
        case 13: //S_IFDOOR
            c = 'D';
            break;
        case 15: //S_IFWHT
            c = 'w';
            break;
        default:
            c = 'u';
            break;
        }
        break;
    }

    return c;
}

char * getPermissions(struct stat sb) {
    char * result = malloc(10 * sizeof(char));
    strcpy(result, "---------");

    int val = sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

    if (val & 256) {
        result[0] = 'r';
    }
    if (val & 128) {
        result[1] = 'w';
    }
    if (val & 64) {
        result[2] = 'x';
    }
    if (val & 32) {
        result[3] = 'r';
    }
    if (val & 16) {
        result[4] = 'w';
    }
    if (val & 8) {
        result[5] = 'x';
    }
    if (val & 4) {
        result[6] = 'r';
    }
    if (val & 2) {
        result[7] = 'w';
    }
    if (val & 1) {
        result[8] = 'x';
    }


    return result;
}

char * getUser(struct stat sb) {
    struct passwd * pw;

    if (!(pw = getpwuid(sb.st_uid))) {
        char * str = malloc(16);
        sprintf(str, "%d", (int)sb.st_uid);
        return str;
    }

    return pw->pw_name;
}

char * getGroup(struct stat sb) {
    struct group * gr;

    if (!(gr = getgrgid(sb.st_gid))) {
        char * str;
        sprintf(str, "%d", (int)sb.st_gid);
        return str;
    }

    return gr->gr_name;
}
