// Minimal test of mmap system call in assembly
    .text
    .align 4
    .global _test_mmap_minimal
_test_mmap_minimal:
    // Test mmap system call
    mov x0, xzr                      // addr = NULL
    mov x1, #4096                    // length = 4096 bytes
    mov x2, #3                       // prot = PROT_READ | PROT_WRITE
    mov x3, #0x1002                 // flags = MAP_PRIVATE | MAP_ANON
    mov x4, #-1                      // fd = -1
    mov x5, xzr                      // offset = 0
    mov x8, #197                     // mmap syscall number (macOS ARM64)
    svc #0                           // Make system call
    
    // Check for mmap failure
    cmp x0, #-1                      // mmap returns -1 on failure
    b.eq mmap_failed                 // Handle mmap failure
    
    // mmap succeeded, try munmap
    mov x19, x0                      // Save allocated address
    mov x0, x19                      // addr = allocated address
    mov x1, #4096                    // length = 4096 bytes
    mov x8, #73                      // munmap syscall number (macOS ARM64)
    svc #0                           // Make system call
    
    // Check for munmap failure
    cmp x0, #-1                      // munmap returns -1 on failure
    b.eq munmap_failed               // Handle munmap failure
    
    // Both succeeded
    mov x0, #1                       // Return success
    ret

mmap_failed:
    mov x0, #0                       // Return failure
    ret

munmap_failed:
    mov x0, #0                       // Return failure
    ret
