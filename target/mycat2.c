#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>


ssize_t get_blksize(){
    static ssize_t blksize = -1;
    if (blksize == -1) {
        blksize = sysconf(_SC_PAGESIZE);
        if (blksize == -1) {
            blksize = 4096; // 默认值
            fprintf(stderr, "Warning: Failed to get page size, using default %ld\n", blksize);
        }
    }
    return blksize;

}


int main(int argc , char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <filename> \n " ,argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1] , O_RDONLY);
    if(fd == -1 ){
        fprintf(stderr, " error opening file %s : %s", argv[1] , strerror(errno) );
        exit(EXIT_FAILURE);
    }
    char c;
    ssize_t readnums ,writenums;

    ssize_t blksize = get_blksize();
    char* buffer = malloc(blksize);
    if(buffer == NULL){
        fprintf(stderr,"Failed to alloc memories: %s" ,strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }
    while( readnums = read(fd , buffer , blksize) >0){
        char* bufp = buffer;
        ssize_t remain = blksize;
        while(remain>0){
            writenums = write(STDOUT_FILENO , bufp, blksize);
            if(writenums == -1){
                fprintf(stderr, "error writing to stdout : %s" , strerror(errno));
                close(fd);
                exit(EXIT_FAILURE);
            }
            remain -= writenums;
            bufp+=writenums;
        }  
    }
    
    if(readnums == -1){
        fprintf(stderr, " error reading file %s : %s" , argv[1] , strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }
    free(buffer);
    if (close(fd) == -1) {
        fprintf(stderr, "Error closing file %s: %s\n", argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}