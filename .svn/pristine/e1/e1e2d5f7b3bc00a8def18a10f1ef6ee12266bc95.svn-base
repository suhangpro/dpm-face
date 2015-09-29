CC=g++
CFLAGS= -std=c++11 -O3 -Wall -pedantic `pkg-config --cflags opencv`
LDFLAGS_BLAS=-lblas
LDFLAGS=-lpthread `pkg-config --libs opencv`
SOURCES=main.cpp eHbbox.cpp eHfeatpyramid.cpp eHimageFeature.cpp eHutils.cpp eHbox.cpp eHfilter.cpp eHmatrix.cpp eHfacemodel.cpp eHimage.cpp eHshiftdt.cpp eHposemodel.cpp
HEADERS=eHbbox.h eHfeatpyramid.h eHimageFeature.h eHutils.h eHbox.h eHfilter.h eHmatrix.h eHfacemodel.h eHimage.h eHshiftdt.h eHposemodel.cpp
EXECUTABLE=facefinder

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE) : $(SOURCES)
	$(CC) -o $(EXECUTABLE) $(SOURCES) $(CFLAGS) $(LDFLAGS) $(LDFLAGS_BLAS) 

clean:
	rm -rf *.o $(EXECUTABLE)



# FEDORA
#LDFLAGS_BLAS=-L /usr/lib64/atlas-sse3/ -lptcblas
#LDFLAGS=-lpthread -lopencv_core -lopencv_highgui
#OBJECTS=$(SOURCES:.cpp=.o)
#$(EXECUTABLE): $(OBJECTS)
#	$(CC) $(LDFLAGS) $(LDFLAGS_BLAS) $(OBJECTS) -o $@

#.cpp.o:
#	$(CC) $(CFLAGS) $< -o $@

#clean: 
#	rm -rf *.o $(EXECUTABLE)
