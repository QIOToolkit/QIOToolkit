sudo apt upgrade -y
sudo apt install -y gcc libomp-dev unzip cmake autoconf automake libtool curl make libboost-all-dev
git clone https://github.com/protocolbuffers/protobuf.git
cd protobuf
git reset --hard ab840345966d0fa8e7100d771c92a73bfbadd25c
cd cmake
sudo cmake -Dprotobuf_BUILD_TESTS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON .
sudo make
sudo make install
sudo ldconfig

echo "Dependencies downloaded"
echo "Building project."
cd ../cpp
echo $PWD
cmake -DCMAKE_BUILD_TYPE=Debug .
make
echo "Build complete."
echo "Running tests."
make test