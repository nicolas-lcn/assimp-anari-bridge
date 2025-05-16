mkdir build && cd build
cmake .. # -DCMAKE_PREFIX_PATH="path/to/anari;path/to/assimp"
./test_bridge model.obj
