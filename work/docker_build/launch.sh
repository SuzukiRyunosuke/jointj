#!/usr/bin/env bash
USER=ubuntu
BASEDIR=$(dirname $0)
cd $BASEDIR/../..
docker run \
  -it \
  -d \
  --name joint \
  --mount type=bind,source="$(pwd)",target=/home/$USER/polyfem \
  joint \
  /usr/bin/bash
