if [ -d $HOME/Downloads ]; then
  cd $HOME/Downloads
elif [ -d $HOME/downloads ]; then
  cd $HOME/downloads
fi
git clone https://github.com/rui314/mold.git
mkdir mold/build
cd mold/build
git checkout v2.4.0
../install-build-deps.sh
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=c++ ..
cmake --build . -j4
sudo cmake --build . --target install
