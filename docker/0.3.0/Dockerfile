FROM rockylinux:9.2

RUN set -ex \
    && yum makecache \
    && yum -y update \
    && yum -y install dnf-plugins-core \
    && yum config-manager --set-enabled crb \
    && yum -y install \
           gcc \
           gcc-c++\
           gdb \
           git \
           gnupg \
           make \
           automake \
           libtool \
           file \
           ninja-build \
           json-c-devel \
           libibverbs-devel \
           boost-devel \
           boost-openmpi-devel \
           json-c-devel \
           openmpi-devel \
    # install cmake 3.21+ since we need to produce JUnit XML files
    && curl -OL https://github.com/Kitware/CMake/releases/download/v3.27.6/cmake-3.27.6-Linux-x86_64.sh \
    && chmod u+x ./cmake-3.27.6-Linux-x86_64.sh \
    && ./cmake-3.27.6-Linux-x86_64.sh --skip-license --prefix=/usr \
    # cleanup
    && yum clean all \
    && rm -rf /var/cache/yum \
    && rm ./cmake-3.27.6-Linux-x86_64.sh

# Download and install dependencies
RUN set -ex \
    && export LD_LIBRARY_PATH=${DEPS_INSTALL_PATH}/lib:${DEPS_INSTALL_PATH}/lib64 \
    && export PKG_CONFIG_PATH=${DEPS_INSTALL_PATH}/lib/pkgconfig:${DEPS_INSTALL_PATH}/lib64/pkgconfig \
    && cd  \
    && mkdir deps  \
    && cd deps \
    && git clone https://github.com/ofiwg/libfabric --recurse-submodules \
    && git clone https://github.com/pmodels/argobots --recurse-submodules \
    && git clone https://github.com/mercury-hpc/mercury --recurse-submodules \
    && git clone https://github.com/mochi-hpc/mochi-margo --recurse-submodules \
    && git clone https://github.com/USCiLab/cereal --recurse-submodules \
    && git clone https://github.com/mochi-hpc/mochi-thallium --recurse-submodules \
    \
    && cd \
    ### argobots
    && cd deps/argobots \
    && ./autogen.sh \
    && mkdir build  \
    && cd build \
    && CFLAGS="-ggdb3 -O0" ../configure --prefix=${DEPS_INSTALL_PATH} \
    && make install -j \
    && cd ..  \
    && rm -rf build  \
    && cd \
    \
    ### libfabric
    && cd deps/libfabric \
    && git checkout v1.14.0rc3 \
    && ./autogen.sh \
    && mkdir build  \
    && cd build \
    && CFLAGS="-ggdb3 -O0" ../configure --prefix=${DEPS_INSTALL_PATH} \
    && make install -j \
    && cd ..  \
    && rm -rf build  \
    && cd \
    \
    ### mercury
    && cd deps/mercury  \
    && mkdir build && cd build \
    && cmake  \
          -DMERCURY_USE_SELF_FORWARD:BOOL=ON \
          -DBUILD_TESTING:BOOL=ON \
          -DMERCURY_USE_BOOST_PP:BOOL=ON \
          -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_PATH}  \
          -DBUILD_SHARED_LIBS:BOOL=ON \
          -DNA_USE_OFI:BOOL=ON  \
          -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
          -DCMAKE_BUILD_TYPE:STRING=Debug  \
          ..  \
    && make install -j \
    && cd ..  \
    && rm -rf build  \
    && cd \
    \
    ### mochi-margo
    && cd deps/mochi-margo \
    && ./prepare.sh \
    && mkdir build  \
    && cd build \
    && CFLAGS="-ggdb3 -O0" ../configure --prefix=${DEPS_INSTALL_PATH} \
    && make -j install \
    && cd ..  \
    && rm -rf build  \
    && cd \
    \
    ### cereal
    && cd deps/cereal \
    && mkdir build  \
    && cd build  \
    \
    && cmake  \
          -DCMAKE_BUILD_TYPE:STRING=Debug  \
          -DBUILD_DOC:BOOL=OFF \
          -DBUILD_SANDBOX:BOOL=OFF  \
          -DBUILD_TESTS:BOOL=OFF \
          -DSKIP_PERFORMANCE_COMPARISON:BOOL=ON \
          -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_PATH} \
          .. \
    && make -j install \
    && cd ..  \
    && rm -rf build  \
    && cd \
    \
    ### mochi-thallium
    && cd deps/mochi-thallium \
    && mkdir build  \
    && cd build  \
    && cmake  \
          -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_PATH}  \
          -DCMAKE_BUILD_TYPE:STRING=Debug  \
          .. \
    && make -j install  \
    && cd ..  \
    && rm -rf build  \
    && cd  \
    \
    && rm -rf deps
