# LMStoreLLVM
LLVM LMStore Block Allocation Pass

# Build LLVMPass Project
    $ cd MMLMStore
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ cd ..

# RUN
    $ clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 -O0 ./benchmark/qsort.c 
    $ opt-9 -load build/src/liblmstorePass.so -lmstore -S qsort.ll > qsort1.ll 
    $ clang-9 -fno-pie  -o './out/qsort.out' -std=c99 -Wall -Wextra -pedantic qsort1.ll link.ld
