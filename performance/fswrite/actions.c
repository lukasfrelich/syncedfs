#include "actions.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#define max(m,n) ((m) > (n) ? (m) : (n))

int seqwrite(long count, int mode, const char* srcpath, const char* dstpath) {
    long size;
    FILE *srcfile;
    struct stat srcstat;
    char* data;

    if (stat(srcpath, &srcstat) == -1) {
        perror("stat");
        return -1;
    }

    /*if srcfile is a character */
    if (S_ISCHR(srcstat.st_mode)) {
        size = max(MAX_SRC_SIZE, count);
    } else {
        size = srcstat.st_size;

        if (size < 1) {
            perror("file empty");
            return -1;
        }
    }

    srcfile = fopen(srcpath, "rb");
    if (srcfile == NULL) {
        perror("open source");
        return -1;
    }

    data = (char *) malloc(size + 1);
    if (data == NULL) {
        perror("malloc");
        return -2;
    }

    if (size != fread(data, sizeof (char), size, srcfile)) {
        free(data);
        perror("read");
        return -2;
    }


    int iter = count / size;
    int rem = count % size;
    FILE *dstfile;

    if (mode == MODE_WRITE)
        dstfile = fopen(dstpath, "wb");
    if (mode == MODE_APPEND)
        dstfile = fopen(dstpath, "ab");

    if (dstfile == NULL) {
        perror("open target");
        return -1;
    }

    for (int i = 0; i < iter; i++) {
        if (size != fwrite(data, sizeof (char), size, dstfile)) {
            free(data);
            perror("write");
            return -2;
        }
    }

    if (rem != fwrite(data, sizeof (char), rem, dstfile)) {
        free(data);
        perror("write");
        return -2;
    }

    free(data);
    fclose(srcfile);
    fclose(dstfile);

    return 0;
}

int ranwrite(long count, const char* srcpath, const char* dstpath) {
    long srcsize, dstsize;
    FILE* srcfile;
    FILE* dstfile;
    struct stat srcstat, dststat;
    char data[BLOCK_SIZE];

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
    
    dstsize = dststat.st_size;
    if (dstsize < count) {
        perror("dstfile is smaller than count");
        return -1;
    }
    
    
    /*write blocks in random order*/
    long numblocks = count / BLOCK_SIZE;
    long* blocks = (long*) malloc(numblocks * sizeof(long));
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
    
    
    srcfile = fopen(srcpath, "rb");
    if (srcfile == NULL) {
        perror("open source");
        return -1;
    }
    dstfile = fopen(dstpath, "r+b");
    if (dstfile == NULL) {
        perror("open destination");
        return -1;
    }
    
    long offset;
    for (long i = 0; i < numblocks; i++) {
        if (srcsize - ftell(srcfile) < BLOCK_SIZE) {
            printf("rewind\n");
            rewind(srcfile);
        }
        if (BLOCK_SIZE != fread(&data, sizeof (char), BLOCK_SIZE, srcfile)) {
            perror("read");
            return -1;
        }
        
        offset = blocks[i] * (BLOCK_SIZE + gap);
        if (fseek(dstfile, offset, SEEK_SET) != 0) {
            perror("seek");
            return -1;
        }
        
        if (BLOCK_SIZE != fwrite(&data, sizeof (char), BLOCK_SIZE, dstfile)) {
            perror("write");
            return -1;
        }
    }
    return 0;
}
