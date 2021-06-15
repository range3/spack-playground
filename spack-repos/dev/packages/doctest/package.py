# Copyright 2021 range3 (https://github.com/range3/)
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

from spack import *


class Doctest(CMakePackage):
    """doctest is a header-only new C++ testing framework.
    """

    homepage = "https://github.com/onqtam/doctest"
    url      = "https://github.com/onqtam/doctest/archive/refs/tags/2.4.6.tar.gz"
    git      = "https://github.com/onqtam/doctest.git"

    version('master',    branch='master')
    version('2.4.6',     sha256='39110778e6baf373ef04342d7cb3fe35da104cb40769103e8a2f0035f5a5f1cb')

    depends_on('cmake@3:')

    def cmake_args(self):
        spec = self.spec

        args = [
            self.define('DOCTEST_WITH_TESTS', False)
        ]

        return args
