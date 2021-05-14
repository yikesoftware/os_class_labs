// gcc zcp.c -o zcp --static
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int s_fd = -1, t_fd = -1;
    int copy_size = 0;
    struct stat s_statbuf;

    if (argc != 3) {
        fprintf(stderr, "Usage: zcp <Source File> <Target File>.\n");
        exit(-1);
    }
    if (access(argv[1], R_OK) != 0) {
    SOURCE_ACCESS_DENIED:
        fprintf(stderr, "ERROR: Can not access source file: %s!\n", argv[1]);
        exit(-1);
    }

    if ((s_fd = open(argv[1], O_RDONLY)) < 0) {
        goto SOURCE_ACCESS_DENIED;
    }

    if ((t_fd = open(argv[2], O_RDWR | O_CREAT)) < 0) {
        fprintf(stderr, "ERROR: Can not access target file: %s!\n", argv[2]);
        exit(-1);
    }

    // get stat
    stat(argv[1], &s_statbuf);
    copy_size = s_statbuf.st_size;

    // set uid gid mode
    if (chown(argv[2], s_statbuf.st_uid, s_statbuf.st_gid) != 0) {
        fprintf(stderr, "ERROR: Can not set the owner of: %s!\n", argv[2]);
    }
    if (chmod(argv[2], s_statbuf.st_mode) != 0) {
        fprintf(stderr, "ERROR: Can not set the st_mode of: %s!\n", argv[2]);
    }

    // zero copy
    fprintf(stdout, "Copied size: %ld bytes.\n",
            sendfile(t_fd, s_fd, 0, copy_size));

    return 0;
}