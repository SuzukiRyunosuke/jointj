# syntax=docker/dockerfile:1
FROM ubuntu:24.04 as base
# install basic apt packages
RUN \
    apt-get update && \
    apt-get install \
    -y --no-install-recommends --no-install-suggests \
        git \
        openssh-client \
        build-essential \
        cmake \
        python3 \
        curl \
        ca-certificates

#=======================================================================================
FROM base as python 
# install apt package to build python
RUN \
    apt-get install \
    -y --no-install-recommends --no-install-suggests \
        libbz2-dev \
        libdb-dev \
        libffi-dev \
        libgdbm-dev \
        liblzma-dev \
        libncursesw5-dev \
        libreadline-dev \
        libsqlite3-dev \
        libssl-dev \
        tk-dev \
        uuid-dev \
        zlib1g-dev
# install python3.10 (b.c. 'bpy' package seems only supported for python of that version)
RUN cd /tmp && \
    curl -O https://www.python.org/ftp/python/3.10.12/Python-3.10.12.tgz && \
    tar -xzf Python-3.10.12.tgz && \
    cd Python-3.10.12 && \
    ./configure --enable-optimizations && \
    make -j4 altinstall

#=======================================================================================
FROM base as mold
# Build mold
RUN cd /tmp && \
    git clone https://github.com/rui314/mold.git && \
    mkdir mold/build && \
    cd mold/build && \
    git checkout v2.4.0 && \
    ../install-build-deps.sh && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=c++ .. && \
    cmake --build . -j4 && \
    cmake --install . --prefix /opt/mold

#=======================================================================================
FROM base as polyfem_base
ENV USER ubuntu
ENV HOME /home/$USER
# copy files from previous stages and the current directory of polyfem
COPY --from=python /usr/local/bin/python3.10 /usr/local/bin/python3.10
COPY --from=python /usr/local/lib/python3.10 /usr/local/lib/python3.10
COPY --chown=$USER ./ $HOME/polyfem
# change working python version
RUN update-alternatives --install /usr/bin/python python /usr/local/bin/python3.10 1 && \
    update-alternatives --install /usr/bin/python python /usr/bin/python3.12 2 && \
    update-alternatives --set python /usr/local/bin/python3.10
# change user to non-root
WORKDIR $HOME
USER $USER
# setup shell
RUN \
    cd $HOME/polyfem/BML/to_build && \
    ./shell_setup.sh && \
    . $HOME/.bashrc

#=======================================================================================
FROM polyfem_base as polyfem_builder
# copy files from previous stages
COPY --from=mold --chown=$USER /opt/mold $HOME/.local
# change user to non-root
# build polyfem
RUN \
    cd $HOME/polyfem/BML/to_build && \
    ./cmake_init.sh && \
    ./compile.sh

#=======================================================================================
FROM polyfem_base as polyfem_runner
USER root
# install apt packages needed for python packages (especially for gmsh & bpy)
RUN apt install \
    -y --no-install-recommends --no-install-suggests \
        libglu1 \
        libgl1 \
        libxrender1 \
        libxcursor1 \
        libxft-dev \
        libxi-dev \
        libxkbcommon-dev
# change user to non-root
USER $USER
# install python packages into python venv
RUN python -m venv .venv && \
    .venv/bin/pip install \
       numpy \
       matplotlib \
       pandas \
       gmsh \
       bpy

#=======================================================================================
FROM polyfem_runner as polyfem
# copy files from polyfem_builder
  # when you only need the program to be run.
COPY --from=polyfem_builder --chown=$USER $HOME/.local $HOME/.local
  # furthermore, when you want to develop the codebase repeating edit & compile.
#COPY --from=polyfem_builder --chown=$USER $HOME/polyfem/build $HOME/polyfem/build
USER root
# change password (this may not be done if publishing image.)
RUN echo root:root | chpasswd && \
    echo $USER:$USER | chpasswd
# change user to non-root
WORKDIR $HOME
USER $USER
