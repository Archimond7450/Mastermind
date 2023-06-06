FROM mcr.microsoft.com/devcontainers/cpp:0-ubuntu-22.04

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

RUN apt-get update && apt-get install -y --no-install-recommends libx11-dev xserver-xorg-dev xorg-dev gcc g++ gdb cmake

RUN git clone https://github.com/renatoGarcia/icecream-cpp icecream-cpp && \
    cd icecream-cpp && \
    mkdir build && \
    cd build && \
    cmake .. && \
    cmake --install . && \
    cd ../.. && \
    rm -r icecream-cpp
