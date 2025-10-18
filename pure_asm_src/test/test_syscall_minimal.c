// Minimal test to check if system calls work
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

int main(void) {
    printf("Testing mmap system call...\n");
    
    // Try to allocate 4096 bytes using mmap
    void* ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    
    if (ptr == MAP_FAILED) {
        printf("mmap failed\n");
        return 1;
    }
    
    printf("mmap succeeded: %p\n", ptr);
    
    // Try to deallocate using munmap
    int result = munmap(ptr, 4096);
    if (result == -1) {
        printf("munmap failed\n");
        return 1;
    }
    
    printf("munmap succeeded\n");
    printf("System calls work correctly!\n");
    return 0;
}
