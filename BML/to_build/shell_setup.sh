if [ $BASH ]; then
  echo "shell is bash"
  SHELLRC=$HOME/.bashrc
elif [ $(basename $SHELL) = "zsh" ]; then
  echo "shell is zsh"
  SHELLRC=$HOME/.zshrc
else
  echo "unknown shell. setup by yourself."
  exit 1
fi

if [ ! -d $HOME/.local ]; then
  echo "create ~/.local"
  mkdir $HOME/.local
  mkdir $HOME/.local/bin
fi
if [ $(echo $PATH | grep -q "$HOME/.local/bin" && echo 1) ]; then
  echo "path was correctly set"
else
  echo "add ~/.local/bin to path"
  export PATH=$HOME/.local/bin:$PATH
  echo "export PATH=\$HOME/.local/bin:\$PATH" >> $SHELLRC
fi
