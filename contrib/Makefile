# Makefile for libsync

TARGET=libsmltcontrib.so

CFILES= sim.cpp\
        sim_wrapper.cpp\

OBJS += $(patsubst %.cpp,%.o,$(CFILES))
HEADERS=$(wildcard *.h)

COMMONFLAGS +=  -Wall -pthread -fPIC
CXXFLAGS += -std=c++11 $(COMMONFLAGS)
CFLAGS += -std=c99 $(COMMONFLAGS)

CXXFLAGS += $(OPT)
CFLAGS += $(OPT)

DEPS=$(OBJS)

# Json C++
# --------------------------------------------------
JSONBASE = jsoncpp
INC += -I $(JSONBASE)/
DEPS += $(JSONBASE)/jsoncpp.cpp sim.cpp sim_wrapper.cpp
OBJS += $(JSONBASE)/jsoncpp.o

all: $(TARGET)

# Build shared library
# --------------------------------------------------
$(TARGET): $(DEPS) $(EXTERNAL_OBJS)
	$(CXX) -shared $(CXXFLAGS) $(OBJS) $(EXTERNAL_OBJS) -o $(TARGET)
	ar  rcs $(patsubst %.so,%.a,$(TARGET))  $(OBJS) $(EXTERNAL_OBJS)
	cp libsmltcontrib.so ../

# Compile object files
# --------------------------------------------------
%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

%.o : %.c
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean:
	rm -f *.o $(TARGET) 

debug:
	echo $(HEADERS)
