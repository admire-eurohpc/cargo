# cargo

Cargo is a HPC data staging service that runs alongside applications helping 
them to transfer data in parallel between local and shared storage tiers.

> **Note**
>
> This software was partially supported by the EuroHPC-funded project ADMIRE
>  (Project ID: 956748, https://www.admire-eurohpc.eu).

## Dependencies

- Margo (tested with v0.9.8)
- Argobots (tested with v1.1rc2)
- Mercury (tested with v2.1.0rc3)
- Thallium (tested with 5daa9a909d1309620ed874832cf8075644727a8a)
- libboost (tested with 1.71.0)
- MPI (tested with OpenMPI 4.0.3)

## Installation

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
  -DCARGO_BUILD_TESTS:BOOL=ON \
  ..
make -j8 install
```

These commands will generate and install the Cargo server binary
(`${INSTALL_DIR}/bin/cargo`) as well as the Cargo interface
library (`${INSTALL_DIR}/lib/libcargo.so`) and its headers
(`${INSTALL_DIR}/include/cargo/*`).

## Testing

```shell
cd build/tests/
mpirun -np 4 ${INSTALL_DIR}/bin/cargo -C
./tests -S ofi+tcp://127.0.0.1:52000
```
