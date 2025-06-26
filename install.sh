#/bin/bash
#rm -rf build output

cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake --build build --config Release
cmake --install build --prefix ./output --config Release
