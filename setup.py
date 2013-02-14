#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from setuptools import setup, find_packages, Extension, Feature


atom_extensions = Feature(
    description='optional optimized c++ extensions',
    ext_modules=[
        Extension(
            'atom.extensions.catom',
            ['atom/extensions/catom.cpp'],
            language='c++',
        ),
        Extension(
            'atom.extensions.observerpool',
            ['atom/extensions/observerpool.cpp'],
            language='c++',
        ),
        Extension(
            'atom.extensions.eventbinder',
            ['atom/extensions/eventbinder.cpp',
             'atom/extensions/callbackhandler.cpp'],
            language='c++',
        ),
        Extension(
            'atom.extensions.signalbinder',
            ['atom/extensions/signalbinder.cpp',
             'atom/extensions/callbackhandler.cpp'],
            language='c++',
        ),
    ],
)


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
    features={'atom-extensions': atom_extensions}
)

