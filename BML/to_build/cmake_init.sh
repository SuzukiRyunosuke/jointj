if [ ! $POLYFEM_ROODIR ]; then
  git config --global --add safe.directory $HOME/src/polyfem
  export POLYFEM_ROOTDIR=$(git rev-parse --show-toplevel)
fi

POLYFEM_BUILDDIR=$POLYFEM_ROOTDIR/build/
echo "build files location: $POLYFEM_BUILDDIR"
if [ ! -d $POLYFEM_BUILDDIR ]; then
  mkdir $POLYFEM_BUILDDIR
fi

if [ ! $(which cmake) ]; then
  echo "cmake not found. please ask google for 'cmake install' and follow the instruction."
  exit 1
fi

BASEDIR=$(dirname $0)
if [ ! $(which mold) ]; then
  echo "mold not found."
  if [ $# -eq 0 ]
    echo "mold is an alternative linker program which may save your build time."
    read -p "Do you wish to install the program ? enter y or n:" yn
    case $yn in
      [Yy]* )
          if [ ! $(ssh -T github.com)] && [ ! $(ssh -T github)]; then
            echo "unable to connect to github."
            NO_MOLD=1
          else
            $BASEDIR/install_mold.sh;
          fi
          ;;
      * ) NO_MOLD=1; break;;
    esac
  elif [ "$1" == "mold" ]; then
    $BASEDIR/install_mold.sh;
  else
    NO_MOLD=1
  fi
fi

if [ ! $NO_MOLD ]; then
  CMAKE_OPTIONS=$(cat $(find $BASEDIR/cmake_options/*))
else
  CMAKE_OPTIONS=$(cat $(find $BASEDIR/cmake_options/* | grep -v "faster_linker.txt"))
fi

cp $BASEDIR/build.sh $POLYFEM_BUILDDIR
echo "$CMAKE_OPTIONS" > $POLYFEM_BUILDDIR/cmake_options.txt

python $BASEDIR/configure.py
