# Makefile for libsync

TARGET=libsync.so

CFILES=$(wildcard *.c)
OBJS += $(patsubst %.c,%.o,$(CFILES))
HEADERS=$(wildcard *.h)

DEPS = $(OBJS)
DEPS += $(HEADERS)

UMPQ=../umpq/
UMP_OBJS += $(UMPQ)/ump_chan.c \
	$(UMPQ)/ump_conf.c \
	$(UMPQ)/ump_queue.c \
	$(UMPQ)/ump_rxchan.c \
	$(UMPQ)/ump_txchan.c \
	$(UMPQ)/lxaffnuma.c \
	$(UMPQ)/parse_int.c

DEPS += $(UMP_OBJS)
OBJS += $(patsubst %.c,%.o,$(UMP_OBJS))

GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always)

# Switch buildtype: supported are "debug" and "release"
# Do this only if not yet defined (e.g. from parent Makefile)
ifndef BUILDTYPE
	BUILDTYPE := debug
endif

COMMONFLAGS +=  -Wall -pthread -fPIC #-fpic
CXXFLAGS += -std=c++11 $(COMMONFLAGS)
CFLAGS += -std=c99 $(COMMONFLAGS)
INC += -I inc -I $(UMPQ)

ifeq ($(BUILDTYPE),debug)
	OPT=-ggdb -O0 -pg -DSYNC_DEBUG
else
	OPT=-O3
endif

CXXFLAGS += $(OPT)
CFLAGS += $(OPT)

LIBNUMABASE=/mnt/scratch/skaestle/software/numactl-2.0.9/
INC += -I$(LIBNUMABASE)
LIBS += -L$(LIBNUMABASE)

LIBS += -lnuma
CXXFLAGS += -DVERSION=\"$(GIT_VERSION)\" $(COMMONFLAGS)

all: $(TARGET)
test: test/mp-test

# Tests
# --------------------------------------------------
test/mp-test: $(DEPS) $(EXTERNAL_OBJS) test/mp-test.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) test/mp-test.cpp -o $@

# Benchmarks
# --------------------------------------------------
bench/ab-bench: $(DEPS) $(EXTERNAL_OBJS) bench/ab-bench.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/ab-bench.cpp -o $@

# Build shared library
# --------------------------------------------------
$(TARGET): $(DEPS) $(EXTERNAL_OBJS)
	$(CXX) -shared $(CXXFLAGS) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) -o $(TARGET)

# Compile object files
# --------------------------------------------------
%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

# UMPQ code is C, not C++
../umpq/%.o: ../umpq/%.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

%.o : %.c
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean:
	rm -f *.o test/*.o $(TARGET)
	$(MAKE) -C $(UMPQ) clean

debug:
	echo $(HEADERS)

.PHONY: cscope.files
cscope.files:
	find . $(UMPQ) -name '*.[ch]' -or -name '*.cpp' -or -name '*.hpp' > $@

doc: $(HEADERS) $(CFILES)
	doxygen
