import os
import sys
import subprocess
s = subprocess
if "ROOTDIR" not in os.environ:
    os.environ["ROOTDIR"] = \
            s.run(["git", "rev-parse", "--show-toplevel"], check=True, stdout=s.PIPE).stdout.decode("utf-8").rstrip()
root_dir = os.environ["ROOTDIR"]
build_dir = root_dir + "/build/"
with open(build_dir + 'cmake_options.txt') as f:
    options = f.read().splitlines()
cmake_sh = ['cmake', '-B', build_dir, '-S', root_dir ] + options
s.run(cmake_sh, check=True)
