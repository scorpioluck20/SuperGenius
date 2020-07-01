This is the block-lattice super fast cryptotoken system based on the original nanocurrency

# Download SuperGenius project
   
    ```console
    git clone ssh://git@gitlab.geniusventures.io:8487/GeniusVentures/SuperGenius.git --recursive 
    cd SuperGenius
    git checkout develop
    ```
    
# Download thirdparty project

    ```console
    cd ..
    git clone ssh://git@gitlab.geniusventures.io:8487/GeniusVentures/thirdparty.git --recursive 
    cd thirdparty
    git checkout develop
    ```

Then folder structure as follows:

    .
    ├── thirdparty                          # geniustokens thirdparty
    │   ├── grpc                             # grpc latest version (current v1.28.1)
    │   ├── leveldb                          # leveldb latest version
    │   ├── libp2p                           # libp2p cross-compile branch
    │   └── ipfs-lite-cpp                    # current repo
    │        ├── ipfs-lite                   # sub folder
    │        ├── readme.md                   # readme
    │        └── CMakeList.txt               # CMake file
    └── SuperGenius   
        ├── block-lattice-node
        ├── readme.md                       # readme
        └── CMakeList.txt                   # CMake file    
 
# Build on Windows
I used visual studio 2017 to compile SuperGenius project.
1. download Prebuilt-Boost libraries for windows
2. download OpenSSL and install
3. build SuperGenius using following commands in Release configuration:
    
    ```console
    cd SuperGenius 
    md .build 
    cd .build 
    cmake ../build/Windows -G "Visual Studio 15 2017 Win64" \
        -DBUILD_TESTING=OFF \
        -DBOOST_ROOT="C:/local/boost_1_70_0" \
        -DBOOST_INCLUDE_DIR="C:/local/boost_1_70_0" \
        -DBOOST_LIBRARY_DIR="C:/local/boost_1_70_0/lib64-msvc-14.1" \
        -DOPENSSL_ROOT_DIR="C:/Program Files/OpenSSL-Win64" -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release
    ```

if you are going to build and test , then use following commands

    ```console
    cmake ../build/Windows -G "Visual Studio 15 2017 Win64" \
        -DTESTING=ON \
        -DBOOST_ROOT="C:/local/boost_1_70_0" \
        -DBOOST_INCLUDE_DIR="C:/local/boost_1_70_0" \
        -DBOOST_LIBRARY_DIR="C:/local/boost_1_70_0/lib64-msvc-14.1" \
        -DOPENSSL_ROOT_DIR="C:/Program Files/OpenSSL-Win64" -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release
    cd SuperGenius
    ctest -C Release
    ```

To run sepecifi test with detailed log, you can use following commands.

    ```console
    ctest -C Release -R <test_name> --verbose
    ```

To run all tests and display log for failed tests, you can use following commands.

    ```console
    ctest -C Release --output-on-failure
    ```

You can use Debug configuration to debug in Visual Studio.
