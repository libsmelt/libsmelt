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
INC += -I inc -I inc/platforms/linux -I inc/backends 


# versiong
# --------------------------------------------------
GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always)


# flags
# --------------------------------------------------

COMMONFLAGS += -Wpedantic -Werror -Wall -Wfatal-errors \
				-pthread -fPIC -DVERSION=\"$(GIT_VERSION)\" 
CXXFLAGS += -std=c++11 $(COMMONFLAGS)
CFLAGS += -std=gnu99 $(COMMONFLAGS)


# libraries
# --------------------------------------------------
LIBS += -lnuma


# setting the dependencies
# --------------------------------------------------
DEPS = $(OBJS)
DEPS += $(HEADERS)
DEPS += Makefile


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

all: test/nodes-test
#$(TARGET)

test: test/mp-test

# Tests
# --------------------------------------------------
test/mp-test: $(DEPS) $(EXTERNAL_OBJS) test/mp-test.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) test/mp-test.cpp -o $@

test/ping-pong: $(DEPS) $(EXTERNAL_OBJS) test/ping-pong.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) test/ping-pong.cpp -o $@

test/nodes-test: test/nodes-test.c $(TARGET)
	$(CC) $(CFLAGS) $(INC) -L./ test/nodes-test.c -o $@ -lsmltrt

# Benchmarks
# --------------------------------------------------
bench/ab-bench: $(DEPS) $(EXTERNAL_OBJS) bench/ab-bench.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/ab-bench.cpp -o $@

bench/ab-throughput: $(DEPS) $(EXTERNAL_OBJS) bench/ab-throughput.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/ab-throughput.cpp -o $@

bench/pairwise: $(DEPS) $(EXTERNAL_OBJS) bench/pairwise.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/pairwise.cpp -o $@

bench/pairwise_raw: $(DEPS) $(EXTERNAL_OBJS) bench/pairwise_raw.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/pairwise_raw.cpp -o $@

# Build shared library
# --------------------------------------------------
$(TARGET): $(DEPS) $(EXTERNAL_OBJS)
	$(CC) -shared $(CFLAGS) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) -o $(TARGET)
	ar rcs $(patsubst %.so,%.a,$(TARGET)) $(OBJS) $(EXTERNAL_OBJS)

# Compile object files
# --------------------------------------------------
%.o : %.cpp Makefile
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

%.o : %.c Makefile
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -f *.o test/*.o $(TARGET) $(patsubst %.so,%.a,$(TARGET)) test/mp-test

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
