#include "actions.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h> 

#define max(m,n) ((m) > (n) ? (m) : (n))
#define min(m,n) ((m) < (n) ? (m) : (n))

int syncFile(int fd) {
    return fsync(fd);
}

int writeChunk(char *data, long count, long size, int fd) {
    // size is greater than or equal to count
    int iter = count / MAX_WRITE_SIZE;
    int rem = count % MAX_WRITE_SIZE;

    for (int i = 0; i < iter; i++) {
        if (MAX_WRITE_SIZE != write(fd, data + (i * MAX_WRITE_SIZE),
                MAX_WRITE_SIZE)) {
            free(data);
            perror("write");
            return -2;
        }
        if (syncFile(fd) != 0) {
            free(data);
            perror("sync");
            return -2;
        }
    }

    // reminder
    if (rem != write(fd, data, rem)) {
        free(data);
        perror("write");
        return -2;
    }
    if (syncFile(fd) != 0) {
        free(data);
        perror("sync");
        return -2;
    }

    return 0;
}

int seqwrite(long count, int mode, const char* srcpath, const char* dstpath) {
    long srcsize;
    int srcfd, dstfd;
    struct stat srcstat;
    char* data;

    if (stat(srcpath, &srcstat) == -1) {
        perror("stat");
        return -1;
    }

    // if srcfile is a character device (i.e. /dev/urandom)
    if (S_ISCHR(srcstat.st_mode)) {
        srcsize = max(MAX_SRC_SIZE, count);
    } else {
        srcsize = srcstat.st_size;

        if (srcsize < 1) {
            perror("file empty");
            return -1;
        }
    }
    srcsize = min(srcsize, MAX_READ_SIZE);

    srcfd = open(srcpath, O_RDONLY);
    if (srcfd == -1) {
        perror("open source");
        return -1;
    }

    data = (char *) malloc(srcsize + 1);
    if (data == NULL) {
        perror("malloc");
        return -2;
    }

    long a;
    a = read(srcfd, data, srcsize);
    if (a != srcsize) {
        free(data);
        printf("returned bytes %ld, SSIZE_MAX is %ld", a, SSIZE_MAX);
        perror("read");
        return -2;
    }


    int iter = count / srcsize;
    int rem = count % srcsize;

    if (mode == MODE_WRITE)
        dstfd = open(dstpath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (mode == MODE_APPEND)
        dstfd = open(dstpath, O_WRONLY | O_APPEND);

    if (dstfd == -1) {
        perror("open destination");
        return -1;
    }

    // only flush the data at the end, but write small chunks
    for (int i = 0; i < iter; i++) {
        if (writeChunk(data, srcsize, srcsize, dstfd) != 0)
            return -2;
    }

    // remainder
    if (writeChunk(data, rem, srcsize, dstfd) != 0)
        return -2;

    if (syncFile(dstfd) != 0) {
        free(data);
        perror("sync");
        return -2;
    }

    free(data);
    close(srcfd);
    close(dstfd);

    return 0;
}

int ranwrite(long count, const char* srcpath, const char* dstpath) {
    long srcsize, dstsize;
    struct stat srcstat, dststat;
    int srcfd, dstfd;
    char *data;

    if (stat(srcpath, &srcstat) == -1) {
        perror("src stat");
        return -1;
    }
    if (stat(dstpath, &dststat) == -1) {
        perror("dst stat");
        return -1;
    }

    srcsize = srcstat.st_size;
    if (srcsize < BLOCK_SIZE) {
        perror("src file must be at least BLOCK_SIZE long");
        return -1;
    }
    srcsize = min(srcsize, MAX_READ_SIZE);

    dstsize = dststat.st_size;
    if (dstsize < count) {
        perror("dstfile is smaller than count");
        return -1;
    }

    /*write blocks in random order*/
    long numblocks = count / BLOCK_SIZE;
    long* blocks = (long*) malloc(numblocks * sizeof (long));
    long gap = (dstsize == count) ? 0 : (dstsize - count) / numblocks;
    printf("gap: %ld\n", gap);
    if (blocks == NULL) {
        perror("malloc");
        return -1;
    }
    for (long i = 0; i < numblocks; i++) {
        blocks[i] = i;
    }
    /*shuffle*/
    long tmp;
    long rnd;
    for (long i = 0; i < numblocks; i++) {
        rnd = rand() % numblocks;
        tmp = blocks[i];
        blocks[i] = blocks[rnd];
        blocks[rnd] = tmp;
    }


    // preload all the data in memory
    srcfd = open(srcpath, O_RDONLY);
    if (srcfd == -1) {
        perror("open source");
        return -1;
    }
    data = (char *) malloc(srcsize + 1);
    if (data == NULL) {
        perror("malloc");
        return -2;
    }
    if (srcsize != read(srcfd, data, srcsize)) {
        free(data);
        perror("read");
        return -2;
    }

    dstfd = open(dstpath, O_RDWR);
    if (dstfd == -1) {
        perror("open destination");
        return -1;
    }

    long dst_offset;
    long src_offset = 0;
    long unsynced_data = 0;
    for (long i = 0; i < numblocks; i++) {
        if (srcsize - src_offset < BLOCK_SIZE) {
            printf("rewind\n");
            src_offset = 0;
        }

        dst_offset = blocks[i] * (BLOCK_SIZE + gap);
        if (pwrite(dstfd, data + src_offset, BLOCK_SIZE, dst_offset) != BLOCK_SIZE) {
            perror("write");
            return -1;
        }
        src_offset += BLOCK_SIZE;
        unsynced_data += BLOCK_SIZE;

        if (unsynced_data >= MAX_WRITE_SIZE) {
            printf("sync\n");
            if (syncFile(dstfd) != 0) {
                perror("sync");
                return -2;
            }
            unsynced_data = 0;
        }
    }

    if (syncFile(dstfd) != 0) {
        perror("sync");
        return -2;
    }
    free(data);
    close(srcfd);
    close(dstfd);
    return 0;
}
