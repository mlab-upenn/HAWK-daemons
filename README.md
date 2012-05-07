Daemons written in C/C++ to run on the onboard computer of the HAWK quadrotor.

# Dependencies
#### Packages
	sudo apt-get update
    sudo apt-get install cmake
    sudo apt-get install git-core
    sudo apt-get install libusb-1.0-0-dev
    sudo apt-get install freeglut3-dev
    sudo apt-get install libxmu-dev
    sudo apt-get install libxi-dev
    sudo apt-get install libcv-dev
    sudo apt-get install libhighgui-dev

#### zlib
http://www.zlib.net/ 

#### libjpeg
http://libjpeg.sourceforge.net/

#### libfreenect
To install the libfreenect library, first checkout the latest version of the source code from their github repository:

    git clone https://github.com/OpenKinect/libfreenect.git

Next, move into the source folder and build the library using cmake.

    cd libfreenect
    mkdir build
    cd build
    cmake ..

Install it:

    sudo make install

Lastly, ensure you never have to run libfreenect code as an administrator by copying the included udev rules:

    cd ..
    sudo cp platform/linux/udev/51-kinect.rules /etc/udev/rules.d
	
	lsusb

# Compilation
	make
