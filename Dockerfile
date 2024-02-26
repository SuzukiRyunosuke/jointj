# syntax=docker/dockerfile:1
FROM ubuntu:24.04 as builder
RUN \
  apt update && \
  apt install -y --no-install-recommends \
    sudo \
    git \
    openssh-client \
    build-essential \
    cmake \
    vim \
    python3 python-is-python3 python3-pip \
    curl \
    libboost-dev

# Install mold
RUN cd /tmp && \
    git clone https://github.com/rui314/mold.git && \
    mkdir mold/build && \
    cd mold/build && \
    git checkout v2.4.0 && \
    ../install-build-deps.sh && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=c++ .. && \
    cmake --build . -j4 && \
    cmake --build . --target install && \
    rm -rf mold

ENV USER ubuntu
ENV HOME /home/$USER
RUN \
    echo root:root | chpasswd && \
    echo $USER:$USER | chpasswd

COPY --chown=$USER ./ $HOME/polyfem
WORKDIR $HOME
USER $USER
RUN \
    cd $HOME/polyfem/BML/to_build && \
    ./shell_setup.sh && \
    ./cmake_init.sh && \
    ./compile.sh
