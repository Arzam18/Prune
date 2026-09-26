ADDOPTION?=
DEBUGOPT?=
NETNAME?=

myCPPFLAGS=-Werror -Wshadow -Wall -Wextra -MMD -std=gnu++17 -I../Fathom/src $(ADDOPTION)

OPT?=-O3 -flto -funroll-loops

TUNE?=false
HCE?=false
DATAGEN?=false
UNITTEST?=false

SHELL := /bin/bash

# Fathom tablebase library
FATHOM_DIR = ../Fathom/src
FATHOM_OBJ = tbprobe.o

COM=$(shell git rev-parse HEAD)

ARCH?=native
CPUSET?=unknow
ANDROID?=false
EXE?=

SOURCES := $(wildcard *.cpp)

SOURCES := $(filter-out collectData.cpp,$(SOURCES))
SOURCES := $(filter-out engine.cpp,$(SOURCES))
SOURCES := $(filter-out unittest.cpp,$(SOURCES))
SOURCES := $(filter-out netScale.cpp,$(SOURCES))
SOURCES := $(filter-out spsa.cpp,$(SOURCES))
SOURCES := $(filter-out datasetStats.cpp,$(SOURCES))
SOURCES := $(filter-out testTI.cpp,$(SOURCES))
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

ifneq ($(MAIN),engine.cpp)
	BASENAME=$(MAIN:%.cpp=%)
	ADDSUFFIX=false
endif

ifeq ($(MAIN),collectData.cpp)
	BASENAME=coreGen
	MAINFLAGS := -fopenmp
	ADDSUFFIX=false
endif

ifeq ($(HCE),true)
	OBJECTS := $(filter-out NNUE.o,$(OBJECTS))
	myCPPFLAGS := $(myCPPFLAGS) -DHCE
	THENET :=
	BASENAME := pruneHCE
endif

ifeq ($(TUNE),true)
	NOTMAIN := $(NOTMAIN) -DTUNE
	BASENAME := spsa
	MAIN=spsa.cpp
	ADDSUFFIX=false
endif

# ------------------------------------------------------------
# Platform / linker configuration
# ------------------------------------------------------------

myLDFLAGS=

ifeq ($(OS),Windows_NT)

	OS=Windows

	CFLAGS := $(CFLAGS) -static
	OPT := $(OPT) -fuse-ld=lld

	CXX=clang++
	CC=clang

else

	OS=$(shell uname -s)
	PROCESSOR_ARCHITECTURE=$(shell uname -m)

	ifeq ($(ANDROID),true)

		# Android/Bionic provides pthread functionality
		# through libc. Do not add desktop -lpthread.

		myLDFLAGS := $(myLDFLAGS)

	else

		myLDFLAGS := $(myLDFLAGS) -lpthread

	endif

endif

# ------------------------------------------------------------
# Compiler flags
# ------------------------------------------------------------

myCPPFLAGS := $(myCPPFLAGS) $(DEBUGOPT)

ifneq ($(ARCH),unknow)
	ARCHCOM=-march=$(ARCH)
else
	ARCHCOM=
endif

ifneq ($(CPUSET),unknow)
	ARCHCOM := $(ARCHCOM) -m$(CPUSET)
endif

# ------------------------------------------------------------
# Version information
# ------------------------------------------------------------

TAG=$(shell git tag --contains=HEAD)

ifeq ($(TAG),)
	REC=$(strip $(shell git rev-parse --short HEAD))
else
	REC=$(TAG)
endif

ifneq ($(shell git status --porcelain),)
	REC=test
endif

ifeq ($(ARCH),native)
	ARCHNAME=
else
	ARCHNAME=-$(ARCH)
endif

EXTENSION=

VERSIONNING=-DCOMMIT=\"$(COM)\" -DVERSION=\"$(REC)\"

# ------------------------------------------------------------
# Output name
# ------------------------------------------------------------

ifeq ($(EXE),)

	ifeq ($(ADDSUFFIX),true)

		NAME=$(BASENAME)-$(OS)-$(REC)
		SHORTNAME=$(BASENAME)$(REC)

	else

		NAME=$(BASENAME)
		SHORTNAME=$(BASENAME)

	endif

	ifeq ($(OS),Windows)

		NAME := $(NAME).exe
		SHORTNAME := $(SHORTNAME).exe
		EXTENSION = .exe

	endif

else

	NAME=$(EXE)
	SHORTNAME=$(EXE)

endif

ifeq ($(OS),Windows)
	NULL_DEVICE=nul
else
	NULL_DEVICE=/dev/null
endif

# ------------------------------------------------------------
# Network configuration
# ------------------------------------------------------------

ifeq ($(NETNAME),)

	NEEDINGFILE=download
	NETNAME=$(shell tail -1 nn-name.txt)

else

	NEEDINGFILE=

endif

# ------------------------------------------------------------
# Final compiler/linker flags
# ------------------------------------------------------------

override CPPFLAGS += $(myCPPFLAGS) $(NOTMAIN) $(OPT) $(ARCHCOM) $(ADDOPTION)

override LDFLAGS += $(myLDFLAGS)

# ------------------------------------------------------------
# Normal build
# ------------------------------------------------------------

all: $(MAIN) $(OBJECTS)
	$(CXX) $(myCPPFLAGS) $(OPT) $(ARCHCOM) -o $(NAME) $(VERSIONNING) $^ $(LDFLAGS) $(MAINFLAGS)

# ------------------------------------------------------------
# Network download
# ------------------------------------------------------------

download:
	@if [ ! -f model.bin ]; then \
		echo "File missing, downloading..."; \
		curl -OL "https://github.com/tgirolami09/Prune-nets/releases/download/$(shell head -1 nn-name.txt)/$(NETNAME).bin"; \
	elif [ "$$(sha256sum $(NETNAME).bin | cut -c1-7)" != "$$(head -1 nn-name.txt | grep -P '.{7}$$' -o)" ]; then \
		echo "SHA mismatch, re-downloading..."; \
		curl -OL "https://github.com/tgirolami09/Prune-nets/releases/download/$(shell head -1 nn-name.txt)/$(NETNAME).bin"; \
	fi

# ------------------------------------------------------------
# HOST-ONLY permuter
#
# IMPORTANT:
# permute is a network conversion utility.
# It is NOT part of the Android engine.
# The GitHub Actions workflow builds it with host clang++.
# ------------------------------------------------------------

permute: permute.cpp
	clang++ \
		-std=gnu++17 \
		-O3 \
		-flto \
		-funroll-loops \
		-D_CRT_SECURE_NO_WARNINGS \
		-o $@ \
		$<

.PHONY: permute

# ------------------------------------------------------------
# Network dependency
#
# model.bin is generated before the Android build.
# Do NOT make model.bin depend on permute here.
# This prevents Make from rebuilding permute with
# the Android cross compiler.
# ------------------------------------------------------------

$(NETNAME): $(NEEDINGFILE)

model.bin: $(NETNAME)
	./permute $(NETNAME).bin model.bin

# ------------------------------------------------------------
# Main Prune target
# ------------------------------------------------------------

prune: $(MAIN) $(OBJECTS)
	$(CXX) \
		$(myCPPFLAGS) \
		$(OPT) \
		$(ARCHCOM) \
		-o $(SHORTNAME) \
		$^ \
		$(VERSIONNING) \
		$(LDFLAGS) \
		$(MAINFLAGS)

# ------------------------------------------------------------
# Dependency files
# ------------------------------------------------------------

-include $(DEPENDS)

# ------------------------------------------------------------
# Embedded NNUE network
# ------------------------------------------------------------

embeder.o: embeder.cpp $(THENET)
	$(CXX) \
		$(myCPPFLAGS) \
		-O3 \
		$(ARCHCOM) \
		-c \
		$(VERSIONNING) \
		-o $@ \
		embeder.cpp

# ------------------------------------------------------------
# Clean
#
# Do NOT remove permute or model.bin.
# ------------------------------------------------------------

clean:
	rm -f $(OBJECTS) $(DEPENDS)

# ------------------------------------------------------------
# Rebuild
# ------------------------------------------------------------

re:
	$(MAKE) clean
	$(MAKE) all

# ------------------------------------------------------------
# Fathom tablebases
# ------------------------------------------------------------

$(FATHOM_OBJ): $(FATHOM_DIR)/tbprobe.c
	$(CC) \
		-c \
		-O3 \
		$(ARCHCOM) \
		-D_CRT_SECURE_NO_WARNINGS \
		-I$(FATHOM_DIR) \
		-o $@ \
		$<
