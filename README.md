# LQTControl

**LQTControl** is used to stabilize a laserquantum torus 532 to a certain laser frequency.

## 1. System requirements

The software only runs on Microsoft Windows. To build the program from the source code, Visual Studio is required.


## 2. Installation guide

Since the software requires multiple proprietary libraries to be build and run, no pre-built packages can be distributed. Please build the software from the provided source files or get in contact with the developers.

### Building the software

Clone the LQTControl repository using `git clone` and install the required submodules with `git submodule init` and `git submodule update`.

Please install the following dependencies to the default paths:

- Qt 5.15.2 or higher
- PicoTech SDK

Build the LQTControl project using Visual Studio.


# How to cite

If you use LQTControl in a scientific publication, please cite it with:

> Raimund Schlüßler and others (2018), LQTControl version X.X.X: C++ program to control the cavity temperature of a solid state laser [Software]. Available at https://github.com/BrillouinMicroscopy/LQTControl.

If the journal does not accept ``and others``, you can fill in the missing
names from the [credits file](https://github.com/BrillouinMicroscopy/LQTControl/blob/master/CREDITS).