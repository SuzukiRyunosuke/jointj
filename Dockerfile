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

# ---- python(3.11) をパッケージで導入：安定・高速 ----
FROM base AS python_inst
RUN apt-get update && apt-get install -y --no-install-recommends --no-install-suggests \
      software-properties-common && \
    add-apt-repository -y ppa:deadsnakes/ppa && \
    apt-get update && apt-get install -y --no-install-recommends --no-install-suggests \
      python3.11 python3.11-venv python3.11-distutils ca-certificates && \
    ln -sf /usr/bin/python3.11 /usr/local/bin/python3.11 && \
    /usr/local/bin/python3.11 -m ensurepip && \
    /usr/local/bin/python3.11 -m pip install --no-cache-dir --upgrade pip && \
    rm -rf /var/lib/apt/lists/*

#=======================================================================================
FROM python_inst AS polyfem_base

ENV USER=ubuntu
ENV HOME=/home/$USER

# ユーザー作成（存在していてもOK）
RUN useradd -m -s /bin/bash $USER || true

# 既定 python を 3.11 に固定（python3 はそのままでも可）
RUN update-alternatives --install /usr/bin/python python /usr/local/bin/python3.11 1 && \
    update-alternatives --set python /usr/local/bin/python3.11

WORKDIR $HOME

# ソース一式をコピー（Docker build のコンテキストに合わせて調整）
# COPY --from=python /usr/local/bin/python3.10 /usr/local/bin/python3.11
# COPY --from=python /usr/local/lib/python3.10 /usr/local/lib/python3.11
COPY --chown=$USER . $HOME/polyfem
USER $USER

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
    bash ./cmake_init.sh 2>&1 | tee $HOME/cmake.log && \
    bash ./compile.sh 2>&1 | tee $HOME/build.log

#=======================================================================================
FROM polyfem_base AS polyfem_runner
USER root

# bpy が動くためのランタイム依存（X11/GL系）
RUN apt-get update && apt-get install -y --no-install-recommends --no-install-suggests \
      libgl1 libglu1 libxrender1 libxcursor1 libxft-dev libxi-dev libxkbcommon-dev \
      libxinerama1 libx11-6 libxext6 libxt6 libice6 libsm6 libxrandr2 libxxf86vm1 \
      libxfixes3 && \
    rm -rf /var/lib/apt/lists/*

USER $USER
WORKDIR $HOME

# Python 3.11 venv を作成し，bpy を導入（必要ならバージョン固定）
RUN /usr/local/bin/python3.11 -m venv .venv && \
    .venv/bin/pip install --upgrade pip && \
    .venv/bin/pip install numpy matplotlib pandas gmsh bpy
    # 例：特定版に固定したい場合 ->  .venv/bin/pip install "bpy==4.2.*"

#=======================================================================================
# target stage for development.
FROM polyfem_runner AS dev
# copy files from polyfem_builder
  # when you only need the program to be run.
# COPY --from=polyfem_builder --chown=$USER $HOME/.local $HOME/.local
COPY --from=polyfem_builder --chown=ubuntu /home/ubuntu/.local/bin /home/ubuntu/.local/bin
ENV PATH="/home/ubuntu/.local/bin:${PATH}"
  # furthermore, when you want to develop the codebase repeating edit & build.
COPY --from=polyfem_builder --chown=$USER $HOME/polyfem/build $HOME/polyfem/build
USER root
# RUN set -e; \
#     install -d /usr/local/bin; \
#     for p in \
#       /home/ubuntu/polyfem/build/PolyFEM_bin/polyfem \
#       /home/ubuntu/polyfem/build/polyfem \
#       /home/ubuntu/polyfem/build/Release/polyfem \
#       /home/ubuntu/polyfem/build/PolyFEM_bin \
#     ; do \
#       if [ -x "$p" ]; then ln -sf "$p" /usr/local/bin/polyfem; exit 0; fi; \
#       if [ -d "$p" ] && [ -x "$p/polyfem" ]; then ln -sf "$p/polyfem" /usr/local/bin/polyfem; exit 0; fi; \
#     done; \
#     echo "PolyFEM binary not found under /home/ubuntu/polyfem/build" >&2; exit 1
# change password (this may not be done if publishing image.)
RUN echo root:root | chpasswd && \
    echo $USER:$USER | chpasswd
# change user to non-root
WORKDIR $HOME
USER $USER
