mkdir build && cd build
cmake .. # -DCMAKE_PREFIX_PATH="path/to/anari;path/to/assimp"
cmake --build .
./test_bridge model.obj
