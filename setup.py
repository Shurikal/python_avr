from setuptools import Extension, setup

setup(
    ext_modules=[
        Extension(
            name="avr",  # as it would be imported
                               # may include packages/namespaces separated by `.`

            sources=["src/avr/avrcmodule.c"], # all sources are compiled into a single binary file
            include_dirs=["src/avr"], # include directories
        ),
    ]
)
