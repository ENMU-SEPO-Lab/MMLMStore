# LMStoreLLVM
LLVM LMStore Block Allocation Pass

# Building LLVM

    -   sudo apt install cmake
    -   sudo apt install ninja-build
    -   sudo apt install clang
    -   wget http://releases.llvm.org/3.4.2/llvm-3.4.2.src.tar.gz
    -   tar -xf llvm-3.4.2.src.tar.gz
    -   llvm-3.4.2.src llvm
    -   cd llvm
    -   mkdir build && cd build
        -   Using Make
            - cmake -G "Unix Makefiles" path_to_LLVM_source
            - make
        - Using Ninja
            - cmake -G Ninja -DLLVM_TARGETS_TO_BUILD=host path_to_LLVM_source
            - ninja

# Build LLVMPass Project
    -   cd ../lib/Transforms/
    -   git clone https://github.com/NMSU-PLEASE-Lab/LMStoreLLVM.git
    -   vim CMakeLists.txt
    -   add add_subdirectory(LMStoreLLVM/src/)

# RUN
- One way with the LLVM build
    -   cd path_to_LLVM_build
    -   ninja or make
    -   clang -Xclang -load -Xclang lib/libLMStorePass.so file_to_instrument.c
- Another way
    - clang -S -emit-llvm  foo.c
    - opt -load build/src/libLLVMLiveness.so -liveness -S foo.ll

