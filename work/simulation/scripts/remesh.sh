SCRIPTDIR=$(dirname $0)
cd $SCRIPTDIR/.. && \
  python remesh.py concave_iter_$1 -nopopup && \
  python remesh.py convex_iter_$1 -nopopup
