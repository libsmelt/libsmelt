# Makefile for libsync

# Allows to switch the compiler
ifdef CLANG
	CC=clang
	CXX=clang++
else
	CC=gcc
	CXX=g++
endif

TARGET=libsmltrt.so
INSTALL_DIR=/mnt/scratch/smelt-runtime-system/bench/


# Source files
# --------------------------------------------------
CFILES += $(wildcard src/*.c)

# backend specific source files
CFILES += $(wildcard src/backends/shm/*.c)
CFILES += $(wildcard src/backends/ump/*.c)
CFILES += $(wildcard src/backends/ffq/*.c)

# platform specific source files
CFILES += $(wildcard src/platforms/linux/*.c)

# architecture specific
CFILES += $(wildcard src/arch/*.c)

# creating the object files
OBJS += $(patsubst %.c,%.o,$(CFILES))


# headres
# --------------------------------------------------
HEADERS=$(wildcard inc/*.h)

# platform specific header files
HEADERS += $(wildcard inc/backends/shm/*.h)
HEADERS += $(wildcard inc/backends/ump/*.h)
HEADERS += $(wildcard inc/backends/ffq/*.h)

# platform specific header files
HEADERS += $(wildcard inc/platforms/linux/*.h)

# architecture specific
HEADERS += $(wildcard inc/arch/*.h)


# includes
# --------------------------------------------------
INC += -I inc -I inc/backends -I contrib/
INC += -I inc/backends/ump -I inc/backends/ffq -I inc/backends/shm

# versiong
# --------------------------------------------------
GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always)


# flags
# --------------------------------------------------

COMMONFLAGS += -Wpedantic -Werror -Wall -Wfatal-errors \
	-pthread -fPIC -DSMLT_VERSION=\"$(GIT_VERSION)\"
CXXFLAGS += -std=c++11 $(COMMONFLAGS)
CFLAGS += -std=c99 $(COMMONFLAGS) -D_GNU_SOURCE


# libraries
# --------------------------------------------------
LIBS += -lnuma -lm
LIBS += -L./ -L./contrib/ -lsmltcontrib


# setting the dependencies
# --------------------------------------------------
DEPS = $(OBJS)
DEPS += $(HEADERS)
DEPS += Makefile
DEPS += contrib/libsmltcontrib.so


# Switch buildtype: supported are "debug" and "release"
# Do this only if not yet defined (e.g. from parent Makefile)
ifndef BUILDTYPE
	BUILDTYPE := release
endif


ifeq ($(BUILDTYPE),debug)
	OPT=-ggdb -O0 -pg -DSYNC_DEBUG_BUILD -gdwarf-2
else
	ifeq ($(BUILDTYPE),valgrind)
		OPT=-g -ggdb -O1 -DSYNC_DEBUG_BUILD -gdwarf-2
	else
		OPT=-O3 -ggdb
	endif
endif

CXXFLAGS += $(OPT)
CFLAGS += $(OPT)



#ifdef USE_SHOAL
#	LIBSHOAL=../shoal-library/
#	INC +=  -I$(LIBSHOAL)/inc
#	LIBS += -L$(LIBSHOAL)/shoal/ -lshl -llua5.2 -L$(LIBSHOAL)/contrib/papi-5.3.0/src -lpapi -lpfm
#	LIBS += -lpapi -fopenmp
#	CXXFLAGS += -DSHL
#endif

# Should a custom libnuma be used?

# LIBNUMABASE=/mnt/scratch/skaestle/software/numactl-2.0.9/
# INC += -I$(LIBNUMABASE)
# LIBS += -L$(LIBNUMABASE)
BINS = test/nodes-test \
	test/topo-create-test \
	test/dissem-bar-test \
	test/contrib-lib-test \
	test/shm-queue-test \
	test/queuepair-test \
	test/context-test \
	test/hybrid-context-test \
	test/smlt-mp-test \
	test/channel-test \
	bench/bar-bench \
	bench/ab-bench \
	bench/colbench \
	bench/ab-bench-scale \
	bench/ab-bench_s \
	bench/ab-bench-opt \
	bench/pairwise \
	bench/pingpong \
	bench/polloverhead \
	bench/writeoverhead \
	bench/shm-mp-bench \
	bench/multimessage

all: $(TARGET) $(BINS)
	make -C contrib

test: test/nodes-test \
	test/topo-create-test \
	test/contrib-lib-test
	test/shm-queue-test \
	test/queuepair-test


# Tests
# --------------------------------------------------
test/mp-test: $(DEPS) $(EXTERNAL_OBJS) test/mp-test.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) test/mp-test.cpp -o $@

test/ping-pong: $(DEPS) $(EXTERNAL_OBJS) test/ping-pong.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) test/ping-pong.cpp -o $@

test/nodes-test: test/nodes-test.c $(TARGET)
	$(CC) $(CFLAGS) $(INC) $(LIBS) test/nodes-test.c -o $@ -lsmltrt

test/topo-create-test: test/topo-create-test.c $(TARGET)
	$(CC) $(CFLAGS) $(INC) $(LIBS) test/topo-create-test.c -o $@ -lsmltrt

test/contrib-lib-test: test/contrib-lib-test.c $(TARGET)
	$(CC) $(CFLAGS) $(INC) $(LIBS) test/contrib-lib-test.c -o $@ contrib/libsmltcontrib.so -lsmltrt

test/shm-queue-test: test/shm-queue-test.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) test/shm-queue-test.c -o $@ -lsmltrt
test/queuepair-test: test/queuepair-test.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) test/queuepair-test.c -o $@ -lsmltrt
test/ffq-test: test/ffq-test.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) test/ffq-test.c -o $@ -lsmltrt
test/shmqp-test: test/shmqp-test.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) test/shmqp-test.c -o $@ -lsmltrt
test/context-test: test/context-test.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) test/context-test.c -o $@ -lsmltrt
test/hybrid-context-test: test/hybrid-context-test.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) test/hybrid-context-test.c -o $@ -lsmltrt
test/smlt-mp-test: test/smlt-mp-test.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) test/smlt-mp-test.c -o $@ -lsmltrt
test/channel-test: test/channel-test.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) test/channel-test.c -o $@ -lsmltrt
test/dissem-bar-test: test/dissem-bar-test.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) test/dissem-bar-test.c -o $@ -lsmltrt
# Benchmarks
# --------------------------------------------------

bench/ab-bench: bench/ab-bench.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) bench/ab-bench.c -o $@ -lsmltrt

bench/ab-bench-scale: bench/ab-bench-scale.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) bench/ab-bench-scale.c -o $@ -lsmltrt -lnuma

bench/ab-bench_s: bench/ab-bench.c $(TARGET)
	$(CC) $(CFLAGS) -DPRINT_SUMMARY=1 $(INC) $(LIBS) bench/ab-bench.c -o $@ -lsmltrt

bench/pairwise: $(DEPS) $(EXTERNAL_OBJS) bench/pairwise.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/pairwise.c -o $@

bench/pingpong: $(DEPS) $(EXTERNAL_OBJS) bench/pingpong.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/pingpong.c -o $@

bench/polloverhead: $(DEPS) $(EXTERNAL_OBJS) bench/polloverhead.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/polloverhead.c -o $@

bench/writeoverhead: $(DEPS) $(EXTERNAL_OBJS) bench/writeoverhead.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/writeoverhead.c -o $@

bench/multimessage: $(DEPS) $(EXTERNAL_OBJS) bench/multimessage.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/multimessage.c -o $@

bench/shm-mp-bench: $(DEPS) $(EXTERNAL_OBJS) bench/shm-mp-bench.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/shm-mp-bench.c -o $@

bench/colbench: $(DEPS) $(EXTERNAL_OBJS) bench/colbench.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/colbench.c -o $@

bench/ab-bench-opt: $(DEPS) $(EXTERNAL_OBJS) bench/ab-bench-opt.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/ab-bench-opt.c -o $@

bench/bar-bench: $(DEPS) $(EXTERNAL_OBJS) bench/diss_bar/barrier.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/diss_bar/barrier.c bench/diss_bar/mcs.c -o $@

bench/barrier-throughput: $(DEPS) $(EXTERNAL_OBJS) bench/barrier-throughput.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) -I bench/diss_bar bench/diss_bar/mcs.c \
	bench/barrier-throughput.c -o $@

# Build shared library
# --------------------------------------------------
$(TARGET): $(DEPS) $(EXTERNAL_OBJS)
	$(CXX) -shared $(CXXFLAGS) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) -lm -o $(TARGET)
	ar rcs $(patsubst %.so,%.a,$(TARGET)) $(OBJS) $(EXTERNAL_OBJS)

ifdef CLANG
contrib/libsmltcontrib.so:
	CLANG=1 make -C contrib
else
contrib/libsmltcontrib.so:
	make -C contrib
endif


# Compile object files
# --------------------------------------------------
%.o : %.cpp Makefile
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

%.o : %.c Makefile
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -f src/*.o test/*.o $(TARGET) $(patsubst %.so,%.a,$(TARGET))
	rm -f test/mp-test test/topo-create-test test/contrib-lib-test
	rm -f test/shm-queue-test test/nodes-test test/queuepair-test test/shmqp-test
	rm -f test/context-test bench/ab-bench test/ffq-test
	rm -f src/backends/ffq/*.o src/backends/ump/*.o src/backends/shm/*.o
	rm -f test/smlt-mp-test bench/bar-bench bench/ab-bench-scale
	rm -f test/dissem-bar-test bench/shm-mp-bench bench/colbench
debug:
	echo $(HEADERS)

.PHONY: install
install : $(TARGET) bench/multimessage bench/bar-bench bench/ab-bench bench/pairwise_raw bench/pingpong bench/polloverhead
	cp *.so $(INSTALL_DIR)
	cp bench/bar-bench $(INSTALL_DIR)
	cp bench/ab-bench $(INSTALL_DIR)
	cp bench/ab-bench_s $(INSTALL_DIR)
	cp bench/pairwise_raw $(INSTALL_DIR)
	cp bench/pairwise_raw_s $(INSTALL_DIR)
	cp bench/pingpong $(INSTALL_DIR)
	cp bench/polloverhead $(INSTALL_DIR)
	cp bench/multimessage $(INSTALL_DIR)

.PHONY: cscope.files
cscope.files:
	find . -name '*.[ch]' -or -name '*.cpp' -or -name '*.hpp' > $@

doc: $(HEADERS) $(CFILES)
	doxygen
