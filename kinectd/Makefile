CC=g++
CCFLAGS=-g -O3
INCLUDEDIRS=-I/usr/include/ni
LDFLAGS=-lXnCore -lz -ljpeg
SOURCES=kinectd.cpp
EXECUTABLE=kinectd

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CCFLAGS) $(INCLUDEDIRS) $(LDFLAGS) $(SOURCES) -o $(EXECUTABLE)

clean:
	rm -f ./$(EXECUTABLE) ./*~

