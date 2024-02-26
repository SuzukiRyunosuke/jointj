if [ ! $ROODIR ]; then
  export ROOTDIR=$(git rev-parse --show-toplevel)
fi
cd $ROOTDIR/build && \
  make -j2 PolyFEM_bin && \
  install PolyFEM_bin $HOME/.local/bin
