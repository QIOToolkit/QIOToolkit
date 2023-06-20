#!/usr/bin/env sh

cd ../
mkdir -p release_build
cd release_build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

./app/qiotoolkit -v \
  --solver microsoft.simulatedannealing.qiotoolkit \
  --parameters ../examples/params.sa.json \
  --input ../examples/input-data.ising.json \
  --output ../examples/output.ising.sa.json
