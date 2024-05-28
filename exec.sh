#! /bin/bash

#QSort
#clang -S -emit-llvm -O0 benchmark/qsort.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 -O0 ./benchmark/qsort.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S qsort.ll > qsort1.ll
clang-9 -fno-pie  -o './out/qsort.out' -std=c99 -Wall -Wextra -pedantic qsort1.ll link.ld


#fac.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/fac.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S fac.ll > fac1.ll
clang-9 -fno-pie  -o './out/fac.out' -std=c99 -Wall -Wextra -pedantic fac1.ll link.ld

#fibcall.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/fibcall.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S fibcall.ll > fibcall1.ll
clang-9 -fno-pie  -o './out/fibcall.out' -std=c99 -Wall -Wextra -pedantic fibcall1.ll link.ld

#janne_complex.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/janne_complex.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S janne_complex.ll > janne_complex1.ll
clang-9 -fno-pie  -o './out/janne_complex.out' -std=c99 -Wall -Wextra -pedantic janne_complex1.ll link.ld

#prime.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/prime.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S prime.ll > prime1.ll
clang-9 -fno-pie  -o './out/prime.out' -std=c99 -Wall -Wextra -pedantic prime1.ll link.ld

#select.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/select.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S select.ll > select1.ll
clang-9 -fno-pie  -o './out/select.out' -std=c99 -Wall -Wextra -pedantic select1.ll link.ld

#binary search.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/bs.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S bs.ll > bs1.ll
clang-9 -fno-pie  -o './out/bs.out' -std=c99 -Wall -Wextra -pedantic bs1.ll link.ld

#matmult.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/matmult.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S matmult.ll > matmult1.ll
clang-9 -fno-pie  -o './out/matmult.out' -std=c99 -Wall -Wextra -pedantic matmult1.ll link.ld

#statemate.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/statemate.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S statemate.ll > statemate1.ll
clang-9 -fno-pie  -o './out/statemate.out' -std=c99 -Wall -Wextra -pedantic statemate1.ll link.ld

#cnt.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/cnt.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S cnt.ll > cnt1.ll
clang-9 -fno-pie  -o './out/cnt.out' -std=c99 -Wall -Wextra -pedantic cnt1.ll link.ld

#cover.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/cover.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S cover.ll > cover1.ll
clang-9 -fno-pie  -o './out/cover.out' -std=c99 -Wall -Wextra -pedantic cover1.ll link.ld

#duff.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/duff.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S duff.ll > duff1.ll
clang-9 -fno-pie  -o './out/duff.out' -std=c99 -Wall -Wextra -pedantic duff1.ll link.ld

#expint.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/expint.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S expint.ll > expint1.ll
clang-9 -fno-pie  -o './out/expint.out' -std=c99 -Wall -Wextra -pedantic expint1.ll link.ld

#fdct.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/fdct.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S fdct.ll > fdct1.ll
clang-9 -fno-pie  -o './out/fdct.out' -std=c99 -Wall -Wextra -pedantic fdct1.ll link.ld


#fir.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/fir.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S fir.ll > fir1.ll
clang-9 -fno-pie  -o './out/fir.out' -std=c99 -Wall -Wextra -pedantic fir1.ll link.ld


#insertsort.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/insertsort.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S insertsort.ll > insertsort1.ll
clang-9 -fno-pie  -o './out/insertsort.out' -std=c99 -Wall -Wextra -pedantic insertsort1.ll link.ld


#jfdctint.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/jfdctint.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S jfdctint.ll > jfdctint1.ll
clang-9 -fno-pie  -o './out/jfdctint.out' -std=c99 -Wall -Wextra -pedantic jfdctint1.ll link.ld


#ns.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/ns.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S ns.ll > ns1.ll
clang-9 -fno-pie  -o './out/ns.out' -std=c99 -Wall -Wextra -pedantic ns1.ll link.ld

#nsichneu.c
clang-9 -c -fno-discard-value-names -S -emit-llvm -O0 benchmark/nsichneu.c
opt-9 -load build/src/liblmstorePass.so -lmstore -S nsichneu.ll > nsichneu1.ll
clang-9 -fno-pie  -o './out/nsichneu.out' -std=c99 -Wall -Wextra -pedantic nsichneu1.ll link.ld

#rm *.ll
