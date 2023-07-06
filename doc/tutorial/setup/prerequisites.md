---
title: Prerequisites
description: This tutorial shows the prerequisites needed for qiotoolkit.
topic: article
uid: qiotoolkit.tutorial.setup.prerequisites
---

Prerequisites
=============

qiotoolkit uses the cmake build system and your compiler of choice (g++ being used in the example below).

  * `libomp-dev` is required for multi-threading
  * `swig` and `python3-dev` are used to generate python bindings (optional)

In Linux, you can install these dependencies with

```bash
sudo apt install cmake g++ libomp-dev swig python3-dev
sudo apt-get install -y autoconf automake libtool curl make g++ unzip
git clone https://github.com/protocolbuffers/protobuf.git
cd protobuf
git reset --hard 2514f0bd7da7e2af1bed4c5d1b84f031c4d12c10
cd cmake
sudo cmake -Dprotobuf_BUILD_TESTS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON .
sudo make
sudo make install
sudo ldconfig
```
You can find the script in src/Tools of this repo.

_After installing the prerequisites, you can [build qiotoolkit with cmake](cmake.md)._

# Troubleshooting

## A note about installing the latest cmake
The command 
```bash
sudo apt install cmake
```
You may not get the latest version. In such a scenario please follow the steps below. 

```bash
sudo apt-get purge cmake
sudo apt-get update
sudo apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
sudo apt-get update
sudo apt-get install cmake
```

## You may get an error saying that boost is not available
Please try the following steps
```bash
sudo apt-get install libboost-all-dev
```