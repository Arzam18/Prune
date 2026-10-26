CXX=clang++
CC=clang
# Build-time tools must run on the GitHub Actions host, not on Android.
HOSTCXX ?= c++
HOSTOPT ?= -O2

ADDOPTION?=
DEBUGOPT?=
NETNAME?=
myCPPFLAGS=-Werror -Wshadow -Wall -Wextra -Wno-c++11-narrowing -MMD -std=c++20 -I../Fathom/src $(ADDOPTION)
OPT?=-O3 -flto -funroll-loops

TUNE?=false
NUMA?=false
DATAGEN?=false
UNITTEST?=false

SHELL := /bin/bash
# Fathom tablebase library
FATHOM_DIR = ../Fathom/src
FATHOM_OBJ = tbprobe.o
COM=$(shell git rev-parse HEAD)
ifeq ($(ANDROID), true)
ARCH?=armv8-a+simd
else
ARCH?=native
endif
EXE?=
SOURCES := $(wildcard *.cpp)
SOURCES := $(filter-out collectData.cpp,$(SOURCES))
SOURCES := $(filter-out engine.cpp,$(SOURCES))
SOURCES := $(filter-out unittest.cpp,$(SOURCES))
SOURCES := $(filter-out netScale.cpp,$(SOURCES))
SOURCES := $(filter-out spsa.cpp,$(SOURCES))
SOURCES := $(filter-out datasetStats.cpp,$(SOURCES))
SOURCES := $(filter-out quantise.cpp,$(SOURCES))
SOURCES := $(filter-out permute.cpp,$(SOURCES))
OBJECTS := $(SOURCES:%.cpp=%.o) $(FATHOM_OBJ)
DEPENDS := $(OBJECTS:%.o=%.d)

MAIN?=engine.cpp
MAINFLAGS=
THENET=model.bin
BASENAME=prune
ADDSUFFIX=true
NOTMAIN=
ifeq ($(DATAGEN), true)
	MAIN=collectData.cpp
endif
ifneq ($(MAIN), engine.cpp)
	BASENAME=$(MAIN:%.cpp=%)
	ADDSUFFIX=false
endif
ifeq ($(MAIN), collectData.cpp)
	BASENAME=coreGen
	NOTMAIN := $(NOTMAIN) -DDATAGEN
	MAINFLAGS := -fopenmp
	ADDSUFFIX=false
endif
ifeq ($(TUNE), true)
	NOTMAIN := $(NOTMAIN) -DTUNE
	BASENAME := spsa
	MAIN=spsa.cpp
	ADDSUFFIX=false
endif
ifeq ($(NUMA), true)
	myCPPFLAGS := $(myCPPFLAGS) -DNUMA -lnuma -Wno-unused-command-line-argument
endif

myLDFLAGS=
ifeq ($(OS), Windows_NT)
	OS = Windows
	CFLAGS := $(CFLAGS) -static
	OPT := $(OPT) -fuse-ld=lld
	CXX=clang++
	CC=clang
else
	OS=$(shell uname -s)
	PROCESSOR_ARCHITECTURE=$(shell uname -m)
	# Android provides pthread symbols through libc; NDK has no separate -lpthread.
	ifneq ($(ANDROID), true)
		myLDFLAGS := $(myLDFLAGS) -lpthread
	endif
endif
myCPPFLAGS := $(myCPPFLAGS) $(DEBUGOPT)
ifneq ($(ARCH), unknow)
	ARCHCOM=-march=$(ARCH)
else
	ARCHCOM=
endif
# Android/ARM builds must not inherit host-specific x86 flags.
ifeq ($(ANDROID), true)
	ARCHSIZE=
else
	ARCHSIZE=-march=native
endif
# Android's clang targets are 64-bit without the desktop-only -m64 flag.
ifeq ($(ANDROID), true)
	M64=
else
	M64=-m64
endif
ifeq ($(ARCH), x86-64-v2)
	ARCHSIZE = -DARCHSIZE=128
else ifeq ($(ARCH), x86-64-v3)
	ARCHSIZE = -DARCHSIZE=256
else ifeq ($(ARCH), x86-64-v4)
	ARCHSIZE = -DARCHSIZE=512
endif
TAG=$(shell git tag --points-at=HEAD)
ifeq ($(TAG), )
	REC=$(strip $(shell git rev-parse --short HEAD) )
else
	REC=$(TAG)
endif
ifneq ($(shell git status --porcelain),)
	REC=test
endif
ifeq ($(ARCH), native)
	ARCHNAME=
else
	ARCHNAME=-$(ARCH)
endif

CXX_ISCLANG=$(shell $(CXX) --version | grep -q clang && echo yes || echo no)
ifeq ($(CXX_ISCLANG), yes)
	myCPPFLAGS := $(myCPPFLAGS) -Wno-c23-extensions
endif
EXTENSION=
VERSIONNING=-DCOMMIT=\"$(COM)\" -DVERSION=\"$(REC)\"
ifeq ($(EXE), )
	ifeq ($(ADDSUFFIX), true)
		NAME=$(BASENAME)-$(OS)-$(REC)
		SHORTNAME=$(BASENAME)$(REC)
	else
		NAME=$(BASENAME)
		SHORTNAME=$(BASENAME)
	endif
	ifeq ($(OS), Windows)
		NAME := $(NAME).exe
		SHORTNAME := $(SHORTNAME).exe
		EXTENSION = .exe
	endif
else
	NAME=$(EXE)
	SHORTNAME=$(EXE)
endif
ifeq ($(OS),Windows)
    NULL_DEVICE = nul
else
    NULL_DEVICE = /dev/null
endif
ifeq ($(NETNAME), )
	NEEDINGFILE=download
	NETNAME=$(shell tail -1 nn-name.txt)
else
	NEEDINGFILE=
endif

override CPPFLAGS += $(myCPPFLAGS) $(NOTMAIN) $(OPT) $(ARCHCOM)
override LDFLAGS += $(myLDFLAGS)

all: $(MAIN) $(OBJECTS)
	$(CXX) $(M64) $(myCPPFLAGS) $(OPT) $(ARCHCOM) -o $(NAME) $(VERSIONNING) $^ $(LDFLAGS) $(MAINFLAGS)

download:
	@if [ ! -f $(NETNAME).bin ]; then \
		echo "File missing, downloading..."; \
		curl -OL "https://github.com/tgirolami09/Prune-nets/releases/download/$(shell head -1 nn-name.txt)/$(NETNAME).bin"; \
	elif [ "$$(sha256sum $(NETNAME).bin | cut -c1-7)" != "$$(head -1 nn-name.txt | grep -E ".{7}$$" -o)" ]; then \
		echo "SHA mismatch, re-downloading..."; \
		curl -OL "https://github.com/tgirolami09/Prune-nets/releases/download/$(shell head -1 nn-name.txt)/$(NETNAME).bin"; \
	fi

permute: permute.cpp
	$(HOSTCXX) $(HOSTOPT) -D_CRT_SECURE_NO_WARNINGS -std=gnu++17 -o permute $(VERSIONNING) permute.cpp

$(NETNAME): $(NEEDINGFILE)

model.bin: $(NETNAME) permute
	./permute $(NETNAME).bin model.bin

prune: $(MAIN) $(OBJECTS)
	$(CXX) $(M64) $(myCPPFLAGS) $(OPT) $(ARCHCOM) -o $(SHORTNAME) $^ $(VERSIONNING) $(LDFLAGS) $(MAINFLAGS)

-include $(DEPENDS)

embeder.o: embeder.cpp $(THENET)
	$(CXX) $(M64) $(myCPPFLAGS) -O3 $(ARCHCOM) -c $(VERSIONNING) -o $@ embeder.cpp

clean:
	rm -f $(OBJECTS) $(DEPENDS)

re:
	$(MAKE) clean
	$(MAKE) all

# Compile Fathom tablebase library (C code)
$(FATHOM_OBJ): $(FATHOM_DIR)/tbprobe.c
	$(CC) -c -O3 $(ARCHCOM) -D_CRT_SECURE_NO_WARNINGS -I$(FATHOM_DIR) -o $@ $<
