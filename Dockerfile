# syntax=docker/dockerfile:1
FROM ubuntu:24.04 AS base
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
        wget \
        curl \
        ca-certificates \
        libgmp-dev \
        libtriangle-dev \
        libopenblas-dev \
        libsuperlu-dev \
        liblapack-dev \
        libsuitesparse-dev \
        libboost-all-dev \
        gfortran

#=======================================================================================
FROM base AS python
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


# #── ここから追記 ───────────────────────────────────────────

# # Miniconda のインストール
# RUN cd /tmp && \
#     wget -q https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O miniconda.sh && \
#     bash miniconda.sh -b -p /opt/conda && \
#     rm miniconda.sh && \
#     /opt/conda/bin/conda clean -afy

# ENV PATH="/opt/conda/bin:${PATH}"

# # mkl-include を Conda からインストール（バージョンは 2021.3.0 を指定）
# RUN conda install -y -c anaconda mkl-include=2021.3.0 && \
#     conda clean -afy

# #── 追記ここまで ───────────────────────────────────────────


#=======================================================================================
FROM base AS mold
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
FROM base AS polyfem_base
ENV USER=ubuntu
ENV HOME=/home/$USER
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
FROM polyfem_base AS polyfem_builder
# copy files from previous stages
# COPY --from=mold --chown=$USER /opt/mold $HOME/.local

# build polyfem
RUN \
    cd $HOME/polyfem/BML/to_build && \
    ./cmake_init.sh 2>&1 | tee $HOME/cmake.log && \
    ./compile.sh 2>&1 | tee $HOME/build.log

#=======================================================================================
FROM polyfem_base AS polyfem_runner
USER root
# install apt packages needed for python packages (especially for gmsh & bpy)
RUN \
    apt-get update && \
    apt install \
    -y --no-install-recommends --no-install-suggests \
        wget \
        pip \
        libglu1 \
        libgl1 \
        libxrender1 \
        libxcursor1 \
        libxft-dev \
        libxi-dev \
        libxkbcommon-dev \
        libxinerama1 \
        libx11-6 \
        libxext6 \
        libxt6 \
        libice6

# Blenderのインストール
RUN wget -O blender.tar.xz https://download.blender.org/release/Blender2.93/blender-2.93.1-linux-x64.tar.xz && \
    tar -xf blender.tar.xz -C /usr/local && \
    ln -s /usr/local/blender-2.93.1-linux-x64/blender /usr/local/bin/blender

# Blender環境変数の設定
ENV PATH="/usr/local/bin:${PATH}"

# change user to non-root
USER $USER
# install python packages into python venv
RUN python -m venv .venv && \
    .venv/bin/pip install \
       --upgrade pip \
       numpy \
       matplotlib \
       pandas \
       gmsh
       # bpy==3.6.0


#=======================================================================================
# target stage for development.
FROM polyfem_runner AS dev
# copy files from polyfem_builder
  # when you only need the program to be run.
COPY --from=polyfem_builder --chown=$USER $HOME/.local $HOME/.local
  # furthermore, when you want to develop the codebase repeating edit & build.
COPY --from=polyfem_builder --chown=$USER $HOME/polyfem/build $HOME/polyfem/build
USER root
# change password (this may not be done if publishing image.)
RUN echo root:root | chpasswd && \
    echo $USER:$USER | chpasswd
# change user to non-root
WORKDIR $HOME
USER $USER
