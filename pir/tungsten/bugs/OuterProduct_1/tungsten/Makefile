DEBUG ?=0
BUILDDIR=build
TUNGSTEN_HOME:=$(shell cat TUNGSTEN_HOME)
TUNGSTEN_SRC:=$(wildcard $(TUNGSTEN_HOME)/*.h $(TUNGSTEN_HOME)/*.cc $(TUNGSTEN_HOME)/plasticine/*)
LIB = $(filter-out %main.o, $(wildcard $(TUNGSTEN_HOME)/build/*.o))
CPPFLAGS = --std=c++17 -MMD -flto
CPPFLAGS += -I$(TUNGSTEN_HOME)/parallelstl/include -I$(TUNGSTEN_HOME)/tbb/include
CPPFLAGS += -I$(TUNGSTEN_HOME)/templates -I$(TUNGSTEN_HOME)/plasticine/templates
#CPPFLAGS += -I$(TUNGSTEN_HOME)/DRAMSim2 
CPPFLAGS += -I$(TUNGSTEN_HOME)/ramulator/src -DRAMULATOR 
CPPFLAGS += -I$(TUNGSTEN_HOME)/
CPPFLAGS += -Isrc/
CC=g++
LDFLAGS= -flto -lstdc++fs -L$(TUNGSTEN_HOME)/ramulator -lramulator
ifeq ($(DEBUG), 1)
	LDFLAGS += -g -O0
else
	LDFLAGS += -O3
endif

.PHONY: all cp
all: tungsten

tungsten: $(BUILDDIR)/main.o Makefile
	$(CC) $(BUILDDIR)/*.o $(LIB) $(LDFLAGS) -o tungsten 
	mkdir -p logs

$(BUILDDIR)/%.o: src/%.cc Makefile $(TUNGSTEN_HOME)/tungsten 
	mkdir -p $(BUILDDIR)/
	$(CC) $(LDFLAGS) -c $< -o $(BUILDDIR)/$(notdir $@) $(CPPFLAGS) 

$(TUNGSTEN_HOME)/tungsten: $(TUNGSTEN_SRC)
	make -C ${TUNGSTEN_HOME}

clean:
	rm -f tungsten $(BUILDDIR)/*

cp:
	cp -r $(TUNGSTEN_HOME)/plasticine/resources/* .

-include build/*.d
