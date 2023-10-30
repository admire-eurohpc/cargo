# Copyright 2013-2023 Lawrence Livermore National Security, LLC and other
# Spack Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

# ----------------------------------------------------------------------------
# If you submit this package back to Spack as a pull request,
# please first remove this boilerplate and all FIXME comments.
#
# This is a template package file for Spack.  We've put "FIXME"
# next to all the things you'll want to change. Once you've handled
# them, you can save this file and test your package like this:
#
#     spack install cargo
#
# You can edit this file again by typing:
#
#     spack edit cargo
#
# See the Spack documentation for more information on packaging.
# ----------------------------------------------------------------------------

from spack.package import *


class Cargo(CMakePackage):
    """A parallel data stager for malleable applications."""

    homepage = "https://storage.bsc.es/gitlab/hpc/cargo"
    url = "https://storage.bsc.es/gitlab/hpc/cargo/-/archive/v0.2.0/cargo-v0.2.0.tar.bz2"
    git = "https://storage.bsc.es/gitlab/hpc/cargo.git"

    maintainers("alberto-miranda")

    # available versions
    version("latest", branch="main")
    version("0.1.0", sha256="981d00adefbc2ea530f57f8428bd7980e4aab2993a86d8ae4274334c8f055bdb", deprecated=True)
    version("0.2.0", sha256="fd7fa31891b3961dcb376556ec5fa028bf512d96a7c688a160f9dade58dae36f")
    version("0.4.0", sha256="7a3de25165a6c6ce9dc356634d89f7052f8d2bef")

    # build variants
    variant('build_type',
            default='Release',
            description='CMake build type',
            values=('Debug', 'Release', 'RelWithDebInfo', 'ASan'))
    
    variant('tests',
            default=False,
            description='Build and run Cargo tests')

    variant("ofi",
            default=True,
            when="@0.1.0:",
            description="Use OFI libfabric as transport library")

    variant("ucx",
            default=False,
            when="@0.1.0:",
            description="Use UCX as transport library")


    # general dependencies
    depends_on("cmake@3.19", type='build')
    depends_on("mpi")

    #specific dependencies
    # v0.2.0+
    depends_on("boost@1.71 +mpi", when='@0.2.0:')

    # specific dependencies
    # v0.1.0+
    depends_on("argobots@1.1", when='@0.1.0:')
    depends_on("mochi-margo@0.9.8", when='@0.1.0:')
    depends_on("mochi-thallium@0.10.1", when='@0.1.0:')
    depends_on("boost@1.71 +program_options +mpi", when='@:0.1.0')
    depends_on("boost@1.71 +iostreams", when='@0.1.0: +tests')

    with when("@0.1.0: +ofi"):
        depends_on("libfabric@1.14.0 fabrics=sockets,tcp,rxm")
        depends_on("mercury@2.1.0 +ofi")

    with when("@0.1.0: +ucx"):
        depends_on("ucx@1.12.0")
        depends_on("mercury@2.1.0 +ucx")

    def cmake_args(self):
        """Setup Cargo CMake arguments"""
        cmake_args = [
            self.define_from_variant('CARGO_BUILD_TESTS', 'tests') 
        ]
        return cmake_args

    def check(self):
        """Run tests"""
        with working_dir(self.build_directory):
            make("test", parallel=False)

