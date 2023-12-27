<div align="center">
<h1> Cargo </h1>

[![build status)](https://img.shields.io/gitlab/pipeline-status/hpc/cargo?gitlab_url=https%3A%2F%2Fstorage.bsc.es%2Fgitlab%2F&logo=gitlab)](https://storage.bsc.es/gitlab/hpc/cargo/-/pipelines)
[![latest release](https://storage.bsc.es/gitlab/hpc/cargo/-/badges/release.svg)](https://storage.bsc.es/gitlab/hpc/cargo/-/releases)
[![GitLab (self-managed)](https://img.shields.io/gitlab/license/hpc/cargo?gitlab_url=https%3A%2F%2Fstorage.bsc.es%2Fgitlab)](https://storage.bsc.es/gitlab/hpc/cargo/-/blob/main/COPYING)
[![Language](https://img.shields.io/static/v1?label=language&message=C99%20%2F%20C%2B%2B20&color=red)](https://en.wikipedia.org/wiki/C%2B%2B20)

<p><b>A parallel data staging service for HPC clusters</b></p>

</div>

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

Tests can be run automatically with CTest:

```shell
cd build
ctest -VV --output-on-failure --stop-on-failure -j 8
```

When this happens, a Cargo server with 3 workers is automatically started
(via `mpirun`/`mpiexec`) and stopped (via RPC) so that tests can progress.

Alternatively, during development one may desire to run the Cargo server
manually and then the tests. In this case, the following commands can be used:

```shell
# start the Cargo server with 3 workers. The server will be listening on
# port 62000 and will communicate with workers via MPI messages. The server can
# be stopped with Ctrl+C, `kill -TERM <pid>` or `cargo_shutdown <address>`.)
mpirun -np 4 ${INSTALL_DIR}/bin/cargo -l ofi+tcp://127.0.0.1:62000

# run the tests
cd build
RUNNER_SKIP_START=1 ctest -VV --output-on-failure --stop-on-failure -j 8
```


## Options
Cargo supports the following option:
```
b --blocksize (default is 512). Transfers will use this blocksize in kbytes. 
```

## Utilities
There are a few utility command line programs that can be used to interact with Cargo.

```shell
cli/ccp --server ofi+tcp://127.0.0.1:62000 --input /directory/subdir --output /directorydst/subdirdst --if <method> --of <method> 
```
`--input` and `--output` are required arguments, and can be a directory or a file path.
`--if` and `--of`select the specific transfer method, on V0.4.0 there are many combinations:

`--if or --of` can be: posix, gekkofs, hercules, dataclay, expand and parallel (for MPIIO requests, but only one side is allowed).

Typically you should use posix or parallel and then one specialized adhocfs. Posix is also able to be used with LD_PRELOAD, however
higher performance and flexibility can be obtained using the specific configuration.

On the other hand, MPIIO (parallel) uses normally file locking so there is a performance imapact, and posix is faster (we supose no external modifications are done).

Other commands are `ping`, `shutdown` and `shaping` (for bw control).