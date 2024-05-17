cd thirdparty/yaml-cpp-master/

mkdir build
cd build
cmake .. -DYAML_BUILD_SHARED_LIBS=ON
sudo cmake --build . --target install -j$N_CPU

cd ../../..
