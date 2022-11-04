from setuptools import setup


def main():
    setup(name="avr",
          version="a0.0.0",
          description="A module to emulate the AVR uC",
          author="Chris Rüttimann",
          author_email="tbd@tbd.com",
          ext_modules=[Extension("avr", ["src/avrcmodule.c"])])

if __name__ == "__main__":
    main()
