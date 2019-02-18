# autofacts
Towards Automatically Generating a Sound and Complete Dataset for Evaluating Static Analysis Tools

The goal of the project is to randomly insert **program facts** to test binary static analysis tools.

## Setup
We need LLVM to be present on the system.

### Installing LLVM
> Build LLVM and Clang (if you have already done both, just skip it and jump to step 5)

1) Download [LLVM-6.0.0](http://llvm.org/releases/6.0.0/llvm-6.0.0.src.tar.xz), [clang-6.0.0](http://llvm.org/releases/6.0.0/cfe-6.0.0.src.tar.xz)

2) Unzip the LLVM and Clang source files
```
tar xf llvm-6.0.0.src.tar.xz
tar xf cfe-6.0.0.src.tar.xz
mv cfe-6.0.0.src llvm-6.0.0.src/tools/clang
```

3) Create your target build folder and make
```
mkdir llvm-6.0.0.obj
cd llvm-6.0.0.obj
cmake -DCMAKE_BUILD_TYPE=Debug ../llvm-6.0.0.src (or add "-DCMAKE_BUILD_TYPE:STRING=Release" for releae version)
make -j8  
```

4) Add paths for LLVM and Clang
```
export LLVM_SRC=your_path_to_llvm-6.0.0.src
export LLVM_OBJ=your_path_to_llvm-6.0.0.obj
export LLVM_DIR=your_path_to_llvm-6.0.0.obj
export PATH=$LLVM_DIR/bin:$PATH
```


> Build Facts Inserter

5) Download the SVF source code
```
git clone https://github.com/ucsb-seclab/autofacts autofacts
```

6) Build autofacts using cmake
```
cd autofacts
mkdir Release-build
cd Release-build
cmake ../llvmstuff
make -j4
```

   Debug build
```
cd autofacts
mkdir Debug-build
cd Debug-build
cmake -D CMAKE_BUILD_TYPE:STRING=Debug ../llvmstuff
make -j4
```

## Using Facts Inserter

