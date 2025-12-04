# syntax=docker/dockerfile:1
FROM ubuntu:24.04 AS base

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
        libboost-all-dev

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

FROM python_inst AS polyfem_base

ENV USER=ubuntu
ENV HOME=/home/$USER

RUN useradd -m -s /bin/bash $USER || true

RUN update-alternatives --install /usr/bin/python python /usr/local/bin/python3.11 1 && \
    update-alternatives --set python /usr/local/bin/python3.11

WORKDIR $HOME

COPY --chown=$USER . $HOME/joint
USER $USER

RUN \
    cd $HOME/joint/work/to_build && \
    ./shell_setup.sh && \
    . $HOME/.bashrc

FROM polyfem_base AS polyfem_builder

RUN \
    cd $HOME/joint/work/to_build && \
    bash ./cmake_init.sh && \
    bash ./compile.sh

FROM polyfem_base AS polyfem_runner
USER root

RUN apt-get update && apt-get install -y --no-install-recommends --no-install-suggests \
      libgl1 libglu1 libxrender1 libxcursor1 libxft-dev libxi-dev libxkbcommon-dev \
      libxinerama1 libx11-6 libxext6 libxt6 libice6 libsm6 libxrandr2 libxxf86vm1 \
      libxfixes3 && \
    rm -rf /var/lib/apt/lists/*

USER $USER
WORKDIR $HOME

RUN /usr/local/bin/python3.11 -m venv .venv && \
    .venv/bin/pip install --upgrade pip && \
    .venv/bin/pip install numpy matplotlib pandas gmsh bpy

FROM polyfem_runner AS dev
COPY --from=polyfem_builder --chown=ubuntu /home/ubuntu/.local/bin /home/ubuntu/.local/bin
ENV PATH="/home/ubuntu/.local/bin:${PATH}"
COPY --from=polyfem_builder --chown=$USER $HOME/joint/build $HOME/joint/build
USER root
RUN echo root:root | chpasswd && \
    echo $USER:$USER | chpasswd
WORKDIR $HOME
USER $USER
