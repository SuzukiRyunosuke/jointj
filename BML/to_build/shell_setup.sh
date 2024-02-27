#!/bin/bash -i
if [ $(basename $SHELL) = "zsh" ]; then
  echo "shell is zsh"
  SHELLRC=$HOME/.zshrc
elif [ $BASH ]; then
  echo "shell is bash"
  SHELLRC=$HOME/.bashrc
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
  echo "export PATH=\$HOME/.local/bin:\$PATH" >> $SHELLRC
fi
if [ -d $HOME/.venv ]; then
  # override python version
  echo "export PATH=\$HOME/.venv/bin:\$PATH" >> $SHELLRC
fi
