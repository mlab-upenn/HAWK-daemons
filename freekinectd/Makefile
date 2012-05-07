 #
 #  Title:        Makefile for freekinectd
 #  Revision Number:  2.0 (release)
 #  Date Created:   4/11/12
 #
 #  Authors:
 #    Kevin Conley and Paul Gurniak
 #
 #  Description:
 #
 #    This module serves as the interface between the Kinect,
 #    mounted on the quadrotor UAV, and the base station.  It
 #    acquires RGB and depth (calibrated) data using the
 #    libfreenect driver, compresses all data (using libjpeg
 #    and zlib) and transmits data over a TCP socket to the
 #    base station.
 #
 #  Location of back-up and revision history:
 #
 #    A full backup (and reversion history) for this file can be
 #    found in the github repository:
 #
 #    https://github.com/mlab/HAWK-daemons
 #
 #    Navigate to the respective file, and select "History"
 #

CC=gcc
CCFLAGS=-g -Wall -O3
INCLUDEDIRS=-I/usr/local/include/libfreenect
LDFLAGS=-lfreenect -lfreenect_sync -lz -ljpeg
SOURCES=kinectd.c
EXECUTABLE=kinectd

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CCFLAGS) $(INCLUDEDIRS) $(LDFLAGS) $(SOURCES) -o $(EXECUTABLE)

clean:
	rm -f ./$(EXECUTABLE) ./*~

