SCRIPTDIR=$(dirname $0)
cd $SCRIPTDIR/.. && \
  cp out/angle_distr_parallel.csv out/angle_distr_parallel_till_$1.csv && \
  cp out/parallel.csv out/parallel_till_$1.csv
