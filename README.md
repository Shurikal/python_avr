
# To build and install the package
'''
python -m install cpython

python -m pip install .
'''


# Problem

>>> import avr
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
ImportError: dlopen(/Users/chris/miniforge3/envs/python8bit/lib/python3.11/site-packages/avr.cpython-311-darwin.so, 0x0002): tried: '/Users/chris/miniforge3/envs/python8bit/lib/python3.11/site-packages/avr.cpython-311-darwin.so' (mach-o file, but is an incompatible architecture (have (x86_64), need (arm64e)))
>>> 


❯ file /Users/chris/miniforge3/envs/python8bit/lib/python3.11/site-packages/avr.cpython-311-darwin.so
/Users/chris/miniforge3/envs/python8bit/lib/python3.11/site-packages/avr.cpython-311-darwin.so: Mach-O 64-bit bundle x86_64


### fix

delete build folder and run again

# Useful links

https://packaging.python.org/en/latest/tutorials/packaging-projects/

https://setuptools.pypa.io/en/latest/userguide/ext_modules.html

https://github.com/pypa/cibuildwheel/discussions/997


