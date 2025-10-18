**NEVER use globals in the assembly**
**Compiling on MacOS. _ are added by the compiler/linker for globals in .s files when calls are made from .c files**
**compiled .o files go in lib/bin**
**testing executables go in lib/test**
**all testing source files go in pure_asm_src/test**
**all temporary testing files must begin with debug_test**