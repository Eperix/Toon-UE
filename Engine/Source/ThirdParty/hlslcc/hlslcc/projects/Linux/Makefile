# Copyright Epic Games, Inc. All Rights Reserved.
#
# Makefile for building hlslcc on Linux.
#
# We have to build this library with -fvisibility=hidden to avoid symbol
# conflicts with any version of mesa that might get loaded by the GL
# drivers.

ROOT_DIR = ../../src
TEST_SRCS = $(ROOT_DIR)/hlslcc_test/main.cpp
MAIN_SRCS = $(ROOT_DIR)/hlslcc_exe/main.cpp
LIB_SRCS = $(wildcard $(ROOT_DIR)/hlslcc_lib/*.cpp) \
	$(wildcard $(ROOT_DIR)/hlslcc_lib/*.c) \
	$(wildcard $(ROOT_DIR)/hlslcc_lib/mesa/*.c) \
	$(wildcard $(ROOT_DIR)/hlslcc_lib/mesa/*.cpp) \
	$(wildcard $(ROOT_DIR)/hlslcc_lib/glsl/*.cpp)

LIB_OBJS := $(LIB_SRCS:.cpp=.o)
LIB_OBJS := $(LIB_OBJS:.c=.o)
MAIN_OBJS = $(MAIN_SRCS:.cpp=.o)

# expecting TARGET_ARCH to be set
LIB = ../../lib/Linux/$(TARGET_ARCH)/libhlslcc.a
MAIN = ../../bin/Linux/hlslcc_64

all: lib 
lib: $(LIB)
main: $(MAIN)

DEBUG = 0
CC = $(LINUX_ROOT)/bin/clang
CXX = $(LINUX_ROOT)/bin/clang++
CPPFLAGS = -fPIC -Wno-switch -Wno-unused-value -Wno-deprecated-register -target $(TARGET_ARCH) -I$(ROOT_DIR)/hlslcc_lib -fvisibility=hidden
CXXFLAGS = -std=c++17
ifeq ($(DEBUG),1)
CPPFLAGS += -g -O0 -nostdinc++ -I$(ROOT_DIR)/../../../Unix/LibCxx/include/c++/v1
else
CPPFLAGS += -DNDEBUG -g -O2 -nostdinc++ -I$(ROOT_DIR)/../../../Unix/LibCxx/include/c++/v1
endif


%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $^

%.o: %.c
	$(CC) -x c++ $(CPPFLAGS) -o $@ -c $^

$(LIB): $(LIB_OBJS)
	ar cr $@ $^

$(MAIN): $(MAIN_OBJS) $(LIB)
	$(CXX) $(CPPFLAGS) $^ -L../../lib/Linux/$(TARGET_ARCH) -lhlslcc -o $@ 

clean:
	rm -f $(LIB_OBJS) $(MAIN_OBJS) $(LIB) $(MAIN)

.PHONY: all lib main clean
