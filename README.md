# Cargo

Cargo is a HPC data staging service that runs alongside applications helping 
them to transfer data in parallel between local and shared storage tiers.

> **Note**
>
> This software was partially supported by the EuroHPC-funded project ADMIRE
>  (Project ID: 956748, https://www.admire-eurohpc.eu).


## Building Cargo

### Building Cargo and its dependencies with Spack

Cargo and its dependencies can be built using
[Spack](https://github.com/spack/spack). If you already have Spack, make sure
you have the latest release. If you use a clone of the Spack `develop`
branch, be sure to pull the latest changes.

#### Install Spack

If you haven't already, install Spack with the following commands:

```shell
$ git clone -c feature.manyFiles=true https://github.com/spack/spack
```

This will create a directory called `spack` in your machine. Once you have
cloned Spack, we recommend sourcing the appropriate script for your shell.
This will add Spack to your PATH and enable the use of the `spack` command:

```shell
# For bash/zsh/sh
$ . spack/share/spack/setup-env.sh

# For tcsh/csh
$ source spack/share/spack/setup-env.csh

# For fish
$ . spack/share/spack/setup-env.fish
```

Since `Cargo` is not yet available in the official Spack repositories, you need
to add the Cargo Spack repository to the Spack namespace in your machine. To do
that, download the `spack/` directory in the `Cargo` repository's root to your 
machine (e.g. under `~/projects/cargo/spack`) and execute the following:

```shell
spack repo add ~/projects/cargo/spack/
```

You should now be able to fetch information from the `Cargo` package using
Spack:

```shell
spack info cargo
```

You are now ready to install `Cargo`:

```shell
spack install cargo
```

Include or remove variants with Spack when a custom `Cargo` build is desired.
The available variants are listed below:


| Variant | Command     | Default | Description                        |
|---------|-------------|---------|------------------------------------|
| OFI     | `cargo+ofi` | True    | Use libfabric as transport library |
| UCX     | `cargo+ucx` | False   | Use ucx as transport library       |


> **Attention**
>
> The initial install could take a while as Spack will install build
> dependencies (autoconf, automake, m4, libtool, and pkg-config) as well as
> any dependencies of dependencies (cmake, perl, etc.) if you don’t already
> have these dependencies installed through Spack.

After the installation completes, remember that you first need to load
`Cargo` in order to use it:

```shell
spack load cargo
```

### Building Cargo manually

If you prefer to build and install `Cargo` from sources, you can also do so. 
For the build process to work correctly, the dependencies below will 
need to be available in your system:

| Dependency                                         | Version                   |
|----------------------------------------------------|---------------------------|
| Margo                                              | v0.9.8+                   |
| Argobots                                           | v1.1+                     |
| Mercury                                            | v2.1.0+                   |
| Thallium                                           | v0.10.1+                  |
| libfabric (if `CARGO_TRANSPORT_LIBRARY=libfabric`) | v0.10.1+                  |
| ucx (if `CARGO_TRANSPORT_LIBRARY=ucx`)             | v0.10.1+                  |
| boost program_options                              | v1.71.0+                  |
| boost mpi                                          | v1.71.0+                  |
| boost iostreams (optional, for testing)            | v1.71.0+                  |
| MPI                                                | tested with OpenMPI 4.0.3 |

Once all dependencies are available, you can download build and install
`Cargo` with the following commands:

```shell
## clone the repository
git clone https://storage.bsc.es/gitlab/hpc/cargo.git
cd cargo

## prepare the CMake build
#
# PREFIX="some_dir_where_dependencies_can_be_found"
# INSTALL_DIR="some_dir_where_you_want_cargo_installed"
mkdir build && cd build

## build and install
cmake \
  -DCMAKE_PREFIX_PATH:STRING="${PREFIX};${CMAKE_PREFIX_PATH}" \
  -DCMAKE_INSTALL_PREFIX:STRING="${INSTALL_DIR}" \
  -DCARGO_TRANSPORT_LIBRARY:STRING=libfabric \
  -DCARGO_BUILD_TESTS:BOOL=ON \
  ..
make -j8 install
```

These commands will generate and install the `Cargo` server binary
(`${INSTALL_DIR}/bin/cargo`) as well as the `Cargo` interface
library (`${INSTALL_DIR}/lib/libcargo.so`) and its headers
(`${INSTALL_DIR}/include/cargo/*`).

## Testing

```shell
cd build/tests/
mpirun -np 4 ${INSTALL_DIR}/bin/cargo -C
./tests -S ofi+tcp://127.0.0.1:52000
```
