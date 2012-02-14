Daemons written in C/C++ to run on the onboard computer of the HAWK quadrotor.

# Dependencies
#### Packages
	sudo add-apt-repository "deb http://archive.canonical.com/ lucid partner"
	sudo apt-get update
	sudo apt-get install g++ python libusb-1.0-0-dev freeglut3-dev sun-java6-jdk doxygen 

#### Install OpenNI
	git clone https://github.com/OpenNI/OpenNI.git
	cd OpenNI/Platform/Linux/CreateRedist
	chmod a+x RedistMaker
	./RedistMaker
	cd ../Redist/OpenNI-Bin-Dev-Linux-x86-v1.5.2.23
	sudo ./install.sh

#### Install SensorKinect
	git clone https://github.com/avin2/SensorKinect.git
	cd SensorKinect/Bin
	tar xvf SensorKinect091-Bin-Linux32-v5.1.0.25.tar.bz2
	cd Sensor-Bin-Linux-x86-v5.1.0.25/
	sudo ./install.sh
Verify that three "Microsoft Corp" entries appear when you plug in the Kinect and run:
	
	lsusb

#### Copy Kinect XML Config File*
Download http://www.imaginativeuniversal.com/codesamples/kinectxmls.zip
Copy /KinectXMLs/OpenNI/SamplesConfig.xml to the OpenNI installation folder (/OpenNI/data/), overwriting the existing XML file.

# Installation
	make
	sudo make install