from setuptools import setup
from Cython.Build import cythonize

setup(
    ext_modules=cythonize(
        [], # Add .pyx files here
        compiler_directives={'language_level': '3'}
    )
)
