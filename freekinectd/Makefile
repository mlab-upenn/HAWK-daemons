CC=gcc
CCFLAGS=-g -Wall -O3
INCLUDEDIRS=-I/usr/local/include/libfreenect
LDFLAGS=-lfreenect -lfreenect_sync -lz -ljpeg8
SOURCES=kinectd.c
EXECUTABLE=kinectd

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CCFLAGS) $(INCLUDEDIRS) $(LDFLAGS) $(SOURCES) -o $(EXECUTABLE)

clean:
	rm -f ./$(EXECUTABLE) ./*~

