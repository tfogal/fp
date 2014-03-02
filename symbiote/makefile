MPICC=mpicc
WARN=
DBG:=-g
FLAGS=-qlanglvl=extc99 -qpic=large -q64
CFLAGS=$(FLAGS) $(WARN) $(DBG)
# humorously, dlsym() actually violates the standard
CFLAGS_ORIDE=$(FLAGS) -g $(DBG)
LDFLAGS:=-Wl,--no-undefined $(FLAGS)
LDLIBS=-ldl -lrt
SHARED=-G -qmkshrobj
obj=csv.o simplesitu.o debug.o parallel.mpi.o\
  echo.mpi.o \
  fproc.o posix.o h5.o

all: $(obj) libsitu.so writecsv libecho.so situ

analyze: $(obj)
	clang --analyze $(shell mpicc -showme:compile) -I/usr/include/python2.7 *.c
	rm -f *.plist # clang creates a bunch of these annoying files.
	cppcheck --quiet *.c

writecsv: csv.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

libfp.so: override.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

libsitu.so: debug.o fproc.o h5.o posix.o simplesitu.o
	$(CC) $(SHARED) $^ -o $@ $(LDFLAGS) $(LDLIBS)

libecho.so: debug.o echo.mpi.o parallel.mpi.o
	$(MPICC) $(SHARED) $^ -o $@ $(LDFLAGS) $(LDLIBS)

libmpitee.so: writebin.mpi.o
	$(MPICC) $(SHARED) $^ -o $@ $(LDFLAGS) $(LDLIBS)

libenzo.so: debug.o enzo.mpi.o parallel.mpi.o
	$(MPICC) $(SHARED) $^ -o $@ $(LDFLAGS) $(LDLIBS)

libnek.so: debug.o nek5k.mpi.o parallel.mpi.o png.o
	$(MPICC) $(SHARED) $^ -o $@ $(LDFLAGS) $(LDLIBS) -lpng

modtest: debug.o modified.o testmodified.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

mpiwrapper: runner.mpi.o
	$(MPICC) $(CFLAGS) $^ -o $@ $(LDLIBS)
mpienv: setenv.mpi.o
	$(MPICC) $(CFLAGS) $^ -o $@ $(LDLIBS)
mpifopen: open.mpi.o
	$(MPICC) $(CFLAGS) $^ -o $@ $(LDLIBS)

envpar: parenv.mpi.o
	$(MPICC) $(CFLAGS) $^ -o $@ $(LDLIBS)

%.mpi.o: %.mpi.c
	$(MPICC) -c $(CFLAGS) $< -o $@

situ: forkenv.o evfork.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(obj) \
    libecho.so libenzo.so libfp.so libmpitee.so libnek.so \
    libsitu.so \
    envpar mpienv mpifopen mpiwrapper situ writecsv \
    modtest

override.o: override.c
	$(CC) -c $(CFLAGS_ORIDE) $^ -o $@
simplesitu.o: simplesitu.c
	$(CC) -c $(CFLAGS_ORIDE) $^ -o $@
