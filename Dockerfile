FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

# Path: Dockerfile
RUN apt-get update \
    && apt install -y software-properties-common build-essential \
    gcc libomp-dev unzip cmake \ 
    autoconf automake libtool curl make git ninja-build \
    g++ libomp-dev swig python3-dev python3-pip \
    lcov doxygen graphviz cppcheck sudo wget

# Install boost 1.81
RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz \
    && tar -xzf boost_1_81_0.tar.gz \
    && cd boost_1_81_0 \
    && ./bootstrap.sh --prefix=/usr/local \
    && ./b2 install \
    && cd .. \
    && rm -rf boost_1_81_0.tar.gz boost_1_81_0

# Symlink python to python3
RUN ln -s /usr/bin/python3 /usr/bin/python

RUN pip3 install --upgrade pip \
    && pip3 install gcovr

# Install cmake 3.26.3
RUN apt-get install -y apt-transport-https ca-certificates gnupg wget
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | apt-key add - \
    && apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main' \
    && apt-get update \
    && apt-get install -y cmake

RUN git clone https://github.com/protocolbuffers/protobuf.git \
    && cd protobuf \
    && git checkout v3.20.3 \
    && cd cmake \
    && cmake -Dprotobuf_BUILD_TESTS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON . \
    && make -j$(nproc) \
    && make install \
    && ldconfig \
    && rm -rf /tmp/protobuf

# Install docfx 2.67.5
ARG DOCFX_VERSION=v2.67.5
RUN wget https://github.com/dotnet/docfx/releases/download/${DOCFX_VERSION}/docfx-linux-x64-${DOCFX_VERSION}.zip \
    && unzip docfx-linux-x64-${DOCFX_VERSION}.zip -d /opt/docfx \
    && ln -s /opt/docfx/docfx /usr/local/bin/docfx \
    && rm docfx-linux-x64-${DOCFX_VERSION}.zip

# Set working directory
WORKDIR /workdir

# Start bash shell
CMD ["/bin/bash"]
