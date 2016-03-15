# Makefile for libsync

CC=gcc

TARGET=libsmltrt.so

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
INC += -I inc -I inc/platforms/linux -I inc/backends -I contrib/
INC += -I inc/backends/ump -I inc/backends/ffq -I inc/backends/shm

# versiong
# --------------------------------------------------
GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always)


# flags
# --------------------------------------------------

COMMONFLAGS += -Wpedantic -Werror -Wall -Wfatal-errors \
			   -pthread -fPIC -DVERSION=\"$(GIT_VERSION)\"
CXXFLAGS += -std=c++11 $(COMMONFLAGS)
CFLAGS += -std=gnu99 $(COMMONFLAGS) -D_GNU_SOURCE


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
	OPT=-O3 -ggdb
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


all: $(TARGET) \
	   test/nodes-test \
		 test/topo-create-test \
		 test/contrib-lib-test \
		 test/shm-queue-test \
		 test/queuepair-test \
		 test/ffq-test \
		 test/context-test \
		 test/smlt-mp-test \
		 test/channel-test \
		 bench/bar-bench \
		 bench/ab-bench-new \
		 bench/pairwise \
		 bench/pairwise_raw \
		 bench/pingpong \
		 bench/polloverhead

test: test/nodes-test \
			test/topo-create-test \
			test/contrib-lib-test
			test/shm-queue-test \
			test/queuepair-test \
			test/ffq-test

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
test/smlt-mp-test: test/smlt-mp-test.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) test/smlt-mp-test.c -o $@ -lsmltrt
test/channel-test: test/channel-test.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) test/channel-test.c -o $@ -lsmltrt
# Benchmarks
# --------------------------------------------------

bench/ab-bench-new: bench/ab-bench-new.c $(TARGET)
	$(CC) $(CFLAGS)  $(INC) $(LIBS) bench/ab-bench-new.c -o $@ -lsmltrt

bench/ab-throughput: $(DEPS) $(EXTERNAL_OBJS) bench/ab-throughput.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/ab-throughput.c -o $@

bench/pairwise: $(DEPS) $(EXTERNAL_OBJS) bench/pairwise.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/pairwise.c -o $@

bench/pairwise_raw: $(DEPS) $(EXTERNAL_OBJS) bench/pairwise_raw.c
	$(CC) $(CFLAGS)  $(INC) $(LIBS) bench/pairwise_raw.c -o $@ -lsmltrt

bench/pingpong: $(DEPS) $(EXTERNAL_OBJS) bench/pingpong.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/pingpong.c -lm -o $@

bench/polloverhead: $(DEPS) $(EXTERNAL_OBJS) bench/polloverhead.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/polloverhead.c -lm -o $@

bench/bar-bench: $(DEPS) $(EXTERNAL_OBJS) bench/bar-bench.c
	$(CC) $(CFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/bar-bench.c -lm -o $@
# Build shared library
# --------------------------------------------------
$(TARGET): $(DEPS) $(EXTERNAL_OBJS)
	$(CC) -shared $(CFLAGS) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) -lm -o $(TARGET)
	ar rcs $(patsubst %.so,%.a,$(TARGET)) $(OBJS) $(EXTERNAL_OBJS)

contrib/libsmltcontrib.so:
	make -C contrib


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
	rm -f test/context-test bench/ab-bench-new test/ffq-test
	rm -f src/backends/ffq/*.o src/backends/ump/*.o src/backends/shm/*.o
	rm -f test/smlt-mp-test bench/bar-bench
debug:
	echo $(HEADERS)

.PHONY: cscope.files
cscope.files:
	find . -name '*.[ch]' -or -name '*.cpp' -or -name '*.hpp' > $@

doc: $(HEADERS) $(CFILES)
	doxygen

.PHONY: install
install: bench/ab-bench
	#rsync -av bench/ab-bench gottardo:
