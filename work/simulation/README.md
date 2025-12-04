# Interlocking 2d
created: 2024/02/19  
author: Konishi Shogo  

this directory created with resources & scripts as the production of the bachelor thesis in 2023 by Konishi Shogo.  

You can freely modify them, or, if you want to, can always reproduce the result coming back to the commit of reference implementation (tag: konishi), like:
```sh
git checkout konishi
```

And, follow the instructions below.  

0. Requirements  
    Things you can install with apt command:  
    * git  
    * cmake  
    Things you can install with pip command:  
    * bpy  

1. Build PolyFEM  
```sh
polyfem/$ cd BML/to_build
BML/to_build/$ ./shell_setup.sh
BML/to_build/$ source ~/.$(basename $SHELL)rc
BML/to_build/$ ./cmake_init.sh
BML/to_build/$ ./build.sh
```

2. Run
```sh
polyfem/$ cd BML/examples/interlocking-2d
BML/examples/interlocking-2d$ python run.py
```

If anything you don't know and the internet is not helpful,
please contact me for free (email: gbiifvrc517@gmail.com).
I will deal with them as long as I remember.