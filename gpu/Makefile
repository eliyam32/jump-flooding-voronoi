# General Makefile
# Date: 28/07/2006
# Ricardo Farias: rfarias@ufrj.br

CXX = g++
CC =  gcc

EXECUTABLE = main

VCGDIR = $(HOME)/install/vcglib/

OBJS  = $(EXECUTABLE).o $(VCGDIR)/wrap/gui/trackmode.o $(VCGDIR)/wrap/gui/trackball.o textfile.o shader.o buffer.o rfUtil.o
SRC   = $(OBJ:.o=.cpp)

INCLUDES = -I$(VCGDIR) -I/usr/include -I/include
LIBDIRS  = -L/usr/lib
LIBS     = -lglut -lGL -lGLU -lXext -lX11 -lm -lGLEW

CXXFLAGS    = -g -O3 $(INCLUDES) $(LIBDIRS) -D_BSD_SOURCE -fexpensive-optimizations -Wno-deprecated

$(EXECUTABLE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(EXECUTABLE) $(OBJS) $(LIBS)

depend:
	$(CC) $(CXXFLAGS) -M *.cc > .depend

clean:
	rm -f *.o *~ .depend $(EXECUTABLE)

all: clean depend $(EXECUTABLE)

ifeq (.depend,$(wildcard .depend))
include .depend
endif
