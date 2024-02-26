BASEDIR=$(dirname $0)
docker buildx build \
  -t devenv \
  $BASEDIR/../..
