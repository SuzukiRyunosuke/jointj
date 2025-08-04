if [ ! $ROODIR ]; then
  git config --global --add safe.directory $HOME/polyfem
  export ROOTDIR=$(git rev-parse --show-toplevel)
fi

BUILDDIR=$ROOTDIR/build/
echo "build files location: $BUILDDIR"
if [ ! -d $BUILDDIR ]; then
  mkdir $BUILDDIR
fi

if [ ! $(which cmake) ]; then
  echo "cmake not found. please ask google for 'cmake install' and follow the instruction."
  exit 1
fi

BASEDIR=$ROOTDIR/BML/to_build
if [ ! $(which mold) ]; then
  echo "mold not found."
  if [ $# -eq 0 ]; then
    echo "mold is an alternative linker program which may save your build time."
    read -p "Do you wish to install the program ? enter y or n:" yn
    case $yn in
      [Yy]* )
          if [ ! $(ssh -T github.com)] && [ ! $(ssh -T github)]; then
            echo "unable to connect to github."
            MOLD=0
          else
            $BASEDIR/install_mold.sh && \
            MOLD=1
          fi
          ;;
      * )
          MOLD=0
          ;;
    esac
  else # if mold used is decided by the script's argument; $1
    MOLD=$1
  fi
else
  MOLD=1
fi

if [ $MOLD -eq 1 ]; then
  CMAKE_OPTIONS=$(cat $(find $BASEDIR/cmake_options/*) | grep -v '^ *#')
else # execlude the mold related option
  CMAKE_OPTIONS=$(cat $(find $BASEDIR/cmake_options/* | grep -v "faster_linker.txt") | grep -v '^ *#')
fi

cp $BASEDIR/compile.sh $BUILDDIR
echo "$CMAKE_OPTIONS" > $BUILDDIR/cmake_options.txt

python $BASEDIR/configure.py
