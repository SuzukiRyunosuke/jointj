# Overview

This repository provides code for "Modeling
shape evolution in a system composed of interacting elastic bodies based on
individually assigned and temporally evolving energy landscapes."

The Docker setup is designed to:

* Build a multi-stage Docker image that compiles the project and prepares a runtime environment.
* Provide a convenient workflow (`flight.sh`) to clean, build, launch, and attach to the container in one shot.
* Bind-mount the repository into the container so that you can edit files on the host and run them inside Docker.

---

## Directory layout
```text
joint/
├─ work/
│   ├─ docker_build/  # Docker helper scripts (build / launch / enter / etc.)
│   ├─ simulation/   # Python‑based simulation drivers
│   │  ├─ mesh/       # Input meshes for simulations
│   │  └─ scripts/   # Analysis and postprocessing scripts
│   └─ to_build/     # Build configuration/helpers for PolyFEM
│       └─ cmake_options/
│ 
├─ .github/          
├─ cmake/             # CMake helper files and recipes
│  ├─ polyfem/
│  └─ recipes/
├─ docs/              
│  └─ _doxygen/       
├─ scripts/           
├─ src/               # PolyFEM-based source
│  └─ polyfem/
│      ├─ assembler/
│      ├─ autogen/
│      │  └─ quadrature/
│      ├─ basis/
│      │  ├─ barycentric/
│      │  └─ function/
│      ├─ io/
│      ├─ mesh/
│      │  ├─ collision_proxy/
│      │  ├─ mesh2D/
│      │  ├─ mesh3D/
│      │  └─ remesh/
│      │      └─ wild_remesh/
│      ├─ problem/
│      ├─ quadrature/
│      ├─ refinement/
│      ├─ solver/
│      │  ├─ forms/
│      │  │  ├─ adjoint_forms/
│      │  │  └─ parametrization/
│      │  ├─ line_search/
│      │  └─ problems/
│      ├─ state/
│      ├─ time_integrator/
│      └─ utils/
├─ tests/             
└─ tools/             

```

Docker-related helper scripts are placed under:

```text
work/
  docker_build/
    docker_build.sh
    launch.sh
    enter.sh
    clean.sh
    flight.sh
```

The `Dockerfile` is placed at the repository root.

All commands below assume that you run them from `work/docker_build` unless otherwise noted.

---

## Requirements

* Docker
* Docker Buildx (usually available by default in recent Docker versions)
* A POSIX-compatible shell (bash, etc.)

---

## Docker image

The `Dockerfile` defines a multi-stage build based on `ubuntu:24.04`. In brief, it:

* Installs common build tools and dependencies (e.g. `git`, `cmake`, `build-essential`, etc.).
* Sets up Python 3.11 in a virtual environment and installs scientific packages (`numpy`, `matplotlib`, `pandas`), geometry/graphics tools (`gmsh`, `bpy`), and related dependencies.
* Builds the project in a “builder” stage and prepares a lightweight “runner/dev” stage.
* Creates a non-root default user (`ubuntu`).

The final image is tagged as:

```text
joint
```

---

## Quick start

From the repository root:

```bash
cd work/docker_build
./flight.sh
```

This single command:

1. Removes old containers and the `joint` image.
2. Builds the `joint` Docker image.
3. Launches a container named `joint` with the repository bind-mounted to `/home/ubuntu/polyfem`.
4. Attaches you to an interactive shell inside the container.

---

## Script details

### docker_build.sh

Builds the Docker image `joint` using the repository root as the build context:

```bash
./docker_build.sh
```

This script wraps `docker buildx build`.

---

### launch.sh

Launches a detached container named `joint` and bind-mounts the repository into the container:

```bash
./launch.sh
```

Inside the container, the project will appear at:

```text
/home/ubuntu/polyfem
```

---

### enter.sh

Attaches your terminal to the running `joint` container:

```bash
./enter.sh
```

If the container is stopped but still exists, the script starts it first and then attaches.

---

### clean.sh

Removes stopped/created containers and deletes the `joint` image:

```bash
./clean.sh
```

If no containers are found, Docker may print a harmless warning about missing arguments.

---

### flight.sh

All-in-one convenience script:

```bash
./flight.sh
```

This runs, in order:

1. `clean.sh`
2. `docker_build.sh` (with logging enabled)
3. `launch.sh`
4. `enter.sh`

This script is the recommended entry point for normal use.

---

## Typical workflow

### First setup or after changing the Dockerfile

```bash
cd work/docker_build
./flight.sh
```

### Re-attaching to an existing container

```bash
cd work/docker_build
./enter.sh
```

### Full manual rebuild

```bash
cd work/docker_build
./clean.sh
./docker_build.sh
./launch.sh
./enter.sh
```

---

## Running simulations after setup

After entering the container and once the Docker environment is ready, activate the Python virtual environment and run the simulation:

```bash
cd joint
source .venv/bin/activate
cd work/simulation
python run.py
```

This workflow assumes that:

* The virtual environment has been created during the Docker image build process.
* All required Python dependencies for `run.py` are already installed into `.venv`.

If you start a new shell session inside the container (via `enter.sh`), remember to re-run `source .venv/bin/activate` before executing any simulation scripts.

## Notes

* The repository is bind-mounted into the container, so all file edits on the host are immediately visible inside without rebuilding.
* If you change only source files, there is no need to rebuild the image; just reuse `enter.sh`.
* Rebuild the image when changing system dependencies or the `Dockerfile`.

## Upstream

This repository includes a modified copy of PolyFEM (MIT License): https://github.com/polyfem/polyfem
PolyFEM is used as a third-party component for producing the results in our paper.


(README of PolyFEM)


<h1 align="center">
<a href="https://polyfem.github.io/"><img alt="polyfem" src="https://polyfem.github.io/img/polyfem.png" width="60%"></a>
</h1><br>

[![Build](https://github.com/polyfem/polyfem/actions/workflows/continuous.yml/badge.svg?label=test)](https://github.com/polyfem/polyfem/actions/workflows/continuous.yml)
[![codecov](https://codecov.io/github/polyfem/polyfem/graph/badge.svg?token=ZU9KLLTTDT)](https://codecov.io/github/polyfem/polyfem)
[![Nightly](https://github.com/polyfem/polyfem/actions/workflows/nightly.yml/badge.svg)](https://github.com/polyfem/polyfem/actions/workflows/nightly.yml)
[![Docs](https://github.com/polyfem/polyfem/actions/workflows/docs.yml/badge.svg)](https://polyfem.github.io/polyfem)

PolyFEM is a polyvalent C++ FEM library.

Compilation
-----------

All the C++ dependencies required to build the code are included. It should work on Windows, macOS, and Linux, and it should build out-of-the-box with CMake:

    mkdir build
    cd build
    cmake ..
    make -j4

On Linux, `zenity` is required for the file dialog window to work. On macOS and Windows, the native windows are used directly.


### Optional
The formula for higher-order bases is optionally computed at CMake time using an external python script. Consequently, PolyFEM might requires a working installation of Python and some additional packages to build correctly:

- `numpy` and `sympy` (optional)
- `quadpy` (optional)

Usage
-----

The main executable, `./PolyFEM_bin`, can be called with a GUI or through a command-line interface. Simply run:

    ./PolyFEM_bin

A more detailed documentation can be found on the [website](https://polyfem.github.io/).

Documentation
-------------

The full documentation can be found at [https://polyfem.github.io/](https://polyfem.github.io/)



License
-------

The code of PolyFEM itself is licensed under [MIT License](LICENSE). However, please be mindful of third-party libraries which are used by PolyFEM and may be available under a different license.

Citation
--------

If you use PolyFEM in your project, please consider citing our work:

```bibtex
@misc{polyfem,
  author = {Teseo Schneider and Jérémie Dumas and Xifeng Gao and Denis Zorin and Daniele Panozzo},
  title = {{Polyfem}},
  howpublished = "\url{https://polyfem.github.io/}",
  year = {2019},
}
```

```bibtex
@article{Schneider:2019:PFM,
  author = {Schneider, Teseo and Dumas, J{\'e}r{\'e}mie and Gao, Xifeng and Botsch, Mario and Panozzo, Daniele and Zorin, Denis},
  title = {Poly-Spline Finite-Element Method},
  journal = {ACM Trans. Graph.},
  volume = {38},
  number = {3},
  month = mar,
  year = {2019},
  url = {http://doi.acm.org/10.1145/3313797},
  publisher = {ACM}
}
```

```bibtex
@article{Schneider:2018:DSA,
    author = {Teseo Schneider and Yixin Hu and Jérémie Dumas and Xifeng Gao and Daniele Panozzo and Denis Zorin},
    journal = {ACM Transactions on Graphics},
    link = {},
    month = {10},
    number = {6},
    publisher = {Association for Computing Machinery (ACM)},
    title = {Decoupling Simulation Accuracy from Mesh Quality},
    volume = {37},
    year = {2018}
}
```

Acknowledgments & Funding
--------
The software is being developed in the [Geometric Computing Lab](https://cims.nyu.edu/gcl/index.html) at NYU Courant Institute of Mathematical Sciences and the University of Victoria, Canada.


This work was partially supported by:

* the NSF CAREER award 1652515
* the NSF grant IIS-1320635
* the NSF grant DMS-1436591
* the NSF grant 1835712
* the SNSF grant P2TIP2_175859
* the NSERC grant RGPIN-2021-03707
* the NSERC grant DGECR-2021-00461
* Adobe Research
* nTopology
