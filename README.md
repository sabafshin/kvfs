# kvfs
A User space POSIX compliant file-system based on key-value stores

#### How to clone
If you want to clone the repository with all the submodules:
```text
$ git clone --recurse-submodules https://github.com/ATALOO/kvfs.git
```

#### Requirements
Minimum cmake v3.9, gcc v8.0, tested on Ubuntu 18.04. 
To install dependencies for a debian based linux, use the following script. Requires root.
```text
$ ./install_dependencies.sh 
```

#### How to build
Sample build instructions with LevelDB as backing key value store.
```text
$ mkdir build 
$ cd build
$ cmake DCMAKE_BUILD_TYPE=Release -DBuildWithLevelDB=ON ../
$ cmake --build . -- -j 4
```