#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from setuptools import setup, find_packages, Extension


ext_modules = [
    Extension(
        'atom.catom',
        ['atom/src/catom.cpp',
         'atom/src/member.cpp',
         'atom/src/observerpool.cpp',
         'atom/src/memberfunctions.cpp',
         'atom/src/catommodule.cpp',
         'atom/src/signal.cpp',
         'atom/src/event.cpp'],
        language='c++',
    ),
]


setup(
    name='atom',
    version='0.1.0',
    author='Enthought, Inc',
    author_email='info@enthought.com',
    url='https://github.com/enthought/atom',
    description='Memory efficient Python objects',
    long_description=open('README.md').read(),
    install_requires=['distribute'],
    packages=find_packages(),
    ext_modules=ext_modules,
)

