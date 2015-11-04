# Makefile for libsync

TARGET=libsync.so

OBJS += $(patsubst %.c,%.o,$(wildcard *.c))
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
	BUILDTYPE := release
endif

COMMONFLAGS +=  -Wall -pthread -fPIC #-fpic
CXXFLAGS += -std=c++11 $(COMMONFLAGS)
CFLAGS += -std=c99 $(COMMONFLAGS)

INC += -I inc -I $(UMPQ)

ifeq ($(BUILDTYPE),debug)
	CXXFLAGS +=-ggdb -O2 -pg -DSYNC_DEBUG
	CFLAGS +=-ggdb -O2 -pg -DSYNC_DEBUG
else
	CXXFLAGS += -O3
	CFLAGS += -O3
endif

LIBS += -lnuma
CXXFLAGS += -DVERSION=\"$(GIT_VERSION)\" $(COMMONFLAGS) -std=c++11

all: $(TARGET)
test: test/barrier

test/barrier: $(DEPS) $(EXTERNAL_OBJS) test/barrier.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(OBJS) $(EXTERNAL_OBJS) $(LIBS) test/barrier.cpp -o $@

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
