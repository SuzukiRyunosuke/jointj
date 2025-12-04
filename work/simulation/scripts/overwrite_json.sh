BASEDIR=$(dirname $0)
cd $BASEDIR/..

# overwrite 'state.json'
if [ $1 -eq 0 ]; then
  echo "iter 0 (initial state)"
  # What we do here:
  # 1. change the path of target mesh file
  # 2. and lift up concave part a little bit by setting translation.
  sed -r -e "s/(mesh\": \")(.*\/)?([a-z]*)(_iter_[0-9]*)?\.msh/\1mesh\/\3\.msh/" \
      -e "s/(translation\":) \[.*\]/\1 \[0, 0.001\]/" \
    state.json > tmp.json && \
  mv tmp.json state.json
else
  echo "iter $1 (remeshed state on the calculation progress)"
  # What we do here:
  # 1. change the path of target mesh file
  # 2. and no translation.
  sed -r -e "s/(mesh\": \")(.*\/)?([a-z]*)(_iter_[0-9]*)?\.msh/\1out\/\3_iter_$1\.msh/" \
      -e "s/(translation\":) \[.*\]/\1 \[0, 0\]/" \
    state.json > tmp.json && \
  mv tmp.json state.json
fi

# overwrite 'parallel.json'
sed -r -e "s/(iter\": )[0-9]*/\1$1/" parallel.json > tmp.json && \
mv tmp.json parallel.json
