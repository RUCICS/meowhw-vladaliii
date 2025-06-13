#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

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
    ssize_t readnums;
    while( readnums = read(fd , &c , 1) >0){
        ssize_t  writenums = write(STDOUT_FILENO, &c ,1); 
        if(writenums == -1){
            fprintf(stderr, "error writing to stdout : %s" , strerror(errno));
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    
    if(readnums == -1){
        fprintf(stderr, " error reading file %s : %s" , argv[1] , strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }
    if (close(fd) == -1) {
        fprintf(stderr, "Error closing file %s: %s\n", argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}