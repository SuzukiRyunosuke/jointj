SCRIPTDIR=$(dirname $0)
cd $SCRIPTDIR/.. && \
  polyfem -j $1.json --ns --max_threads 4