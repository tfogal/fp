MPICC=mpicc
WARN=
DBG:=-g
FLAGS=-qlanglvl=extc99 -qpic=large -q64
CFLAGS=$(FLAGS) $(WARN) -I../../ $(DBG)
LDFLAGS:=-Wl,--no-undefined $(FLAGS)
LDLIBS=-ldl -lrt
SHARED=-G -qmkshrobj
obj=ctest.mpi.o netz.mpi.o ../../debug.o ../../parallel.mpi.o
cfanalyze:=$(shell mpicc -showme:compile) -I/usr/include/python2.7

all: $(obj) libnetz.so

analyze: $(obj)
	clang --analyze $(CFLAGS) $(cfanalyze) *.c
	rm -f *.plist # clang creates a bunch of these annoying files.
	cppcheck --quiet *.c

libnetz.so: ../../debug.o netz.mpi.o ../../parallel.mpi.o
	$(MPICC) $(SHARED) $^ -o $@ $(LDFLAGS) $(LDLIBS)

hacktest: ctest.mpi.o ../../debug.o netz.mpi.o ../../parallel.mpi.o
	$(MPICC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

%.mpi.o: %.mpi.c
	$(MPICC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(obj) libnetz.so hacktest
