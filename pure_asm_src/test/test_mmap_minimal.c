// Test the minimal mmap assembly function
#include <stdio.h>

extern int test_mmap_minimal(void);

int main(void) {
    printf("Testing minimal mmap assembly function...\n");
    
    int result = test_mmap_minimal();
    
    if (result == 1) {
        printf("✓ SUCCESS: mmap and munmap worked correctly\n");
    } else {
        printf("✗ FAILED: mmap or munmap failed\n");
    }
    
    return result == 1 ? 0 : 1;
}
