SCRIPTDIR=$(dirname $0)
cd $SCRIPTDIR/.. && \
    mv out/angle_distr_parallel_till_$1.csv out/angle_distr_parallel.csv && \
    mv out/parallel_till_$1.csv out/parallel.csv
