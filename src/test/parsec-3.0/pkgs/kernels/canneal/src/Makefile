# Makefile for parallel simulated annealer

PREFIX=${PARSECDIR}/pkgs/kernels/canneal/inst/${PARSECPLAT}

TARGET=canneal
LIBS:=$(LIBS) -lm

ifdef version
  ifeq "$(version)" "pthreads"
    CXXFLAGS+=-DENABLE_THREADS -pthread
  endif
endif

NLD = $(NAUT)/user/framework/nld-cxx-noelle-ubuntu

all:
	$(CXX) $(CXXFLAGS) annealer_thread.cpp -c -o annealer_thread.o
	$(CXX) $(CXXFLAGS) rng.cpp -c -o rng.o
	$(CXX) $(CXXFLAGS) netlist.cpp -c -o netlist.o
	$(CXX) $(CXXFLAGS) main.cpp -c -o main.o
	$(CXX) $(CXXFLAGS) netlist_elem.cpp -c -o netlist_elem.o
	$(NLD) $(KARAT) *.o -o $(TARGET)

clean:
	rm -f *.o $(TARGET)

install:
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)

