#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

void find(const char *dir_name, const char *pattern, time_t mtime, mode_t perm) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char path[1024];

    if (!(dir = opendir(dir_name))) {
        fprintf(stderr, "Cannot open directory %s: %s\n", dir_name, strerror(errno));
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (stat(path, &statbuf) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            find(path, pattern, mtime, perm);
        } else if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
            // Check for name pattern
            if (strstr(entry->d_name, pattern) != NULL) {
                // Check modification time
                if (mtime == -1 || difftime(statbuf.st_mtime, mtime) < 0) {
                    // Check permissions
                    if (perm == -1 || (statbuf.st_mode & 0777) == (perm & 0777)) {
                        printf("%s\n", path);
                    }
                }
            }
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory> [options]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *dir_name = argv[1];
    const char *pattern = "";
    time_t mtime = -1;
    mode_t perm = -1;

    // Parse optional arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-name") == 0 && i + 1 < argc) {
            pattern = argv[++i];
        } else if (strcmp(argv[i], "-mtime") == 0 && i + 1 < argc) {
            mtime = time(NULL) - atoi(argv[++i]) * 86400; // Convert days to seconds
        } else if (strcmp(argv[i], "-perm") == 0 && i + 1 < argc) {
            perm = strtol(argv[++i], NULL, 8);
        }
    }

    // Perform the search
    find(dir_name, pattern, mtime, perm);

    return 0;
}