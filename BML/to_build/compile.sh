if [ ! $POLYFEM_ROODIR ]; then
  export POLYFEM_ROOTDIR=$(git rev-parse --show-toplevel)
fi
cd $POLYFEM_ROOTDIR/build && \
  make -j2 PolyFEM_bin && \
  install PolyFEM_bin $HOME/.local/bin
