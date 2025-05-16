mkdir -p build && cd build
cmake .. # -DCMAKE_PREFIX_PATH="path/to/anari;path/to/assimp"
cmake --build .
cd -
./build/test/test_bridge bunny.obj
