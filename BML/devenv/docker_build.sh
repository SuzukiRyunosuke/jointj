BASEDIR=$(dirname $0)
#if [ $# -eq 1 ] && [ $1 -eq "dev" ]; then
#  TARGET="--target dev"
#else
#  TARGET="--target release"
#fi

docker buildx build \
  -t devenv \
  $BASEDIR/../..
