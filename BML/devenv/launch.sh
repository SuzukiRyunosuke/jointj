USER=ubuntu
BASEDIR=$(dirname $0)
cd $BASEDIR/../..
docker run \
  -it \
  -d \
  --name devenv \
  --mount type=bind,source="$(pwd)",target=/home/$USER/polyfem \
  devenv \
  /usr/bin/bash
