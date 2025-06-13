
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h> 


// 获取最佳I/O块大小，综合考虑文件系统和内存页大小
size_t io_blocksize(const char *filename) {
    static size_t cached_size = 0;
    
    if (cached_size == 0) {
        struct stat st;
        
        // 首先获取文件系统的块大小
        if (stat(filename, &st) == 0) {
            cached_size = st.st_blksize;
            
            // 验证块大小是否合理（是2的幂且不小于512字节）
            if (cached_size < 512 || (cached_size & (cached_size - 1)) != 0) {
                // 文件系统报告了不合理的块大小，使用默认值
                cached_size = 4096;
            }
        } else {
            // 如果获取失败，使用默认值
            cached_size = 4096;
        }
        
        // 获取内存页大小
        long page_size = sysconf(_SC_PAGESIZE);
        if (page_size > 0) {
            // 取文件系统块大小和内存页大小的最小公倍数
            size_t gcd = cached_size;
            size_t remainder = page_size;
            while (remainder != 0) {
                size_t temp = remainder;
                remainder = gcd % remainder;
                gcd = temp;
            }
            cached_size = (cached_size / gcd) * page_size;
        }
    }
    
    return cached_size;
}

// 分配对齐到内存页的内存
void* align_alloc(size_t size) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    if(page_size<0) page_size =4096;
    // 分配足够大的内存以确保我们能找到一个对齐的区域
    void* mem = malloc(size + page_size - 1 + sizeof(void*));
    if (!mem) return NULL;
    
    // 计算对齐的地址
    uintptr_t addr = (uintptr_t)mem + sizeof(void*) + page_size - 1;
    void* aligned_ptr = (void*)(addr - (addr % page_size));
    
    // 在对齐指针前存储原始指针以便释放
    ((void**)aligned_ptr)[-1] = mem;
    
    return aligned_ptr;
}

// 释放对齐分配的内存
void align_free(void* ptr) {
    if (ptr) {
        void* original_ptr = ((void**)ptr)[-1];
        free(original_ptr);
    }
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

        // 使用fadvise提示顺序访问模式
    if (posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL) == -1) {
        fprintf(stderr, "Warning: fadvise failed: %s\n", strerror(errno));
        // 这不是致命错误，继续执行
    }

    

    ssize_t blksize = 32 * io_blocksize(argv[1]);
    char* buffer = align_alloc(blksize);
    if(buffer == NULL){
        fprintf(stderr,"Failed to alloc memories: %s" ,strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

    ssize_t readnums ,writenums;
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
    align_free(buffer);
    if (close(fd) == -1) {
        fprintf(stderr, "Error closing file %s: %s\n", argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

