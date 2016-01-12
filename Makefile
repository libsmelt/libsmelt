# Makefile for libsync

TARGET=libsync.so

CFILES= \
	barrier.c \
	debug.c \
	linux.c \
	mp.c \
	shm.c \
	sync.c \
	topo.c \
	tree_setup.c \
	shm_queue.c \
	ab.c

OBJS += $(patsubst %.c,%.o,$(CFILES))
HEADERS=$(wildcard *.h)

DEPS = $(OBJS)
DEPS += $(HEADERS)

# Fast Forward
# --------------------------------------------------
# Use Fast-Foward as a Linux message passing backend rather than
# UMPQ.
#USE_FFQ=1

UMPQ=../umpq/
UMP_OBJS += $(UMPQ)/ump_chan.c \
	$(UMPQ)/ump_conf.c \
	$(UMPQ)/ump_queue.c \
	$(UMPQ)/ump_rxchan.c \
	$(UMPQ)/ump_txchan.c \
	$(UMPQ)/lxaffnuma.c \
	$(UMPQ)/parse_int.c

FFQ_OBJS += $(UMPQ)/ffq_conf.c \
	$(UMPQ)/ff_queue.c \
	$(UMPQ)/lxaffnuma.c \
	$(UMPQ)/parse_int.c

GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always)

# Switch buildtype: supported are "debug" and "release"
# Do this only if not yet defined (e.g. from parent Makefile)
ifndef BUILDTYPE
	BUILDTYPE := release
endif

COMMONFLAGS +=  -Wall -pthread -fPIC #-fpic
CXXFLAGS += -std=c++11 $(COMMONFLAGS)
CFLAGS += -std=c99 $(COMMONFLAGS)
INC += -I inc -I $(UMPQ)

ifeq ($(BUILDTYPE),debug)
	OPT=-ggdb -O0 -pg -DSYNC_DEBUG -gdwarf-2
else
	OPT=-O3 -ggdb
endif

CXXFLAGS += $(OPT)
CFLAGS += $(OPT)

# Should a custom libnuma be used?
ifdef OVERRIDENUMA
LIBNUMABASE=/mnt/scratch/skaestle/software/numactl-2.0.9/
INC += -I$(LIBNUMABASE)
LIBS += -L$(LIBNUMABASE)
endif

ifdef USE_FFQ
	CXXFLAGS += -DFFQ
	DEPS += $(FFQ_OBJS)
	OBJS += $(patsubst %.c,%.o,$(FFQ_OBJS))
else
	DEPS += $(UMP_OBJS)
	OBJS += $(patsubst %.c,%.o,$(UMP_OBJS))
endif

LIBS += -lnuma
CXXFLAGS += -DVERSION=\"$(GIT_VERSION)\" $(COMMONFLAGS)

all: $(TARGET)

test: test/mp-test

# Tests
# --------------------------------------------------
test/mp-test: $(DEPS) $(EXTERNAL_OBJS) test/mp-test.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) test/mp-test.cpp -o $@

test/ping-pong: $(DEPS) $(EXTERNAL_OBJS) test/ping-pong.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) test/ping-pong.cpp -o $@

# Benchmarks
# --------------------------------------------------
bench/ab-bench: $(DEPS) $(EXTERNAL_OBJS) bench/ab-bench.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/ab-bench.cpp -o $@

bench/pairwise: $(DEPS) $(EXTERNAL_OBJS) bench/pairwise.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/pairwise.cpp -o $@

bench/pairwise_raw: $(DEPS) $(EXTERNAL_OBJS) bench/pairwise_raw.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) bench/pairwise_raw.cpp -o $@

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
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

%.o : %.c
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean:
	rm -f *.o test/*.o $(TARGET) test/mp-test
	$(MAKE) -C $(UMPQ) clean

debug:
	echo $(HEADERS)

.PHONY: cscope.files
cscope.files:
	find . $(UMPQ) -name '*.[ch]' -or -name '*.cpp' -or -name '*.hpp' > $@

doc: $(HEADERS) $(CFILES)
	doxygen

.PHONY: install
install: bench/ab-bench
	rsync -av bench/ab-bench gottardo:
