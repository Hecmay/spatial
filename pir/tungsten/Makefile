DEBUG ?=0
BUILDDIR=build/
LIBDIR=lib/

PLASTICINE_LIB_DBG=$(LIBDIR)plasticine.o.DBG
PLASTICINE_LIB_PRF=$(LIBDIR)plasticine.o.PRF
PLASTICINE_LIB_OPT=$(LIBDIR)plasticine.o.OPT

SRC=$(wildcard *.cc)
SRC+=$(wildcard plasticine/templates/*.cc)
OBJS = $(addprefix $(BUILDDIR), $(patsubst %.cc, %.o, $(notdir $(SRC))))
OBJS_NOMAIN=$(filter-out %main.o, $(OBJS))

OBJS_OPT = $(addsuffix .OPT, $(OBJS))

OBJS_NOMAIN_DBG=$(addsuffix .DBG, $(filter-out %main.o, $(OBJS)))
OBJS_NOMAIN_PRF=$(addsuffix .PRF, $(filter-out %main.o, $(OBJS)))
OBJS_NOMAIN_OPT=$(addsuffix .OPT, $(filter-out %main.o, $(OBJS)))

TESTS=$(wildcard tests/*.h)
TESTOUT=test_bin
CPPFLAGS = --std=c++2a -MMD -Iparallelstl/include -Itbb/include -Itemplates -Iplasticine/templates/ 
CPPFLAGS += -Iramulator/src -DRAMULATOR -DSIMTIME
CPPFLAGS += -Isparsity/templates
CPPFLAGS += -Ibinlog/include
#CC=g++-8
CC=g++
TBBLIB=libtbb.so.2
TBBBUILD=tbb/build/
LDFLAGS=-L$(wildcard $(TBBBUILD)/*_release) -ltbb -Lramulator -lramulator -lstdc++fs -lpthread #-flto
# ifeq ($(DEBUG), 1)
# else
# endif
DBG_CPPFLAGS = -g -O0 -DDEBUG
DBG_LDFLAGS = -g -O0

PRF_CPPFLAGS = -O3 -g -pg
PRF_LDFLAGS = -O3 -g -pg

OPT_CPPFLAGS = -O3 
OPT_LDFLAGS = -O3 

.PHONY: tbb all clean lint ramulator 
all: tungsten $(PLASTICINE_LIB_DBG) $(PLASTICINE_LIB_PRF) $(PLASTICINE_LIB_OPT) 

tungsten: $(OBJS_OPT) Makefile ramulator
	$(CC) $(BUILDDIR)*.o.OPT $(LDFLAGS) -o tungsten

ramulator:
	cd ramulator; make -j168 libramulator.a; cd -

tbb:
	make -j16 -C tbb

commit.h:
	echo "#ifndef COMMIT_H_" >  $@
	echo "#define COMMIT_H_" >> $@
	echo "#define GITCOMMIT \"$(shell git rev-parse HEAD)\"" >> $@
	echo "#endif  // COMMIT_H_" >> $@

$(BUILDDIR)%.o.DBG: %.cc Makefile commit.h 
	mkdir -p $(BUILDDIR) 
	mkdir -p $(TBBBUILD) 
	$(CC) $(LDFLAGS) $(DBG_LDFLAGS) -c $< -o $@ -I. $(CPPFLAGS) $(DBG_CPPFLAGS)

$(BUILDDIR)%.o.DBG: plasticine/templates/%.cc Makefile commit.h 
	mkdir -p $(BUILDDIR) 
	mkdir -p $(TBBBUILD) 
	$(CC) $(LDFLAGS) $(DBG_LDFLAGS) -c $< -o $@ -I. $(CPPFLAGS) $(DBG_CPPFLAGS)

$(BUILDDIR)%.o.PRF: %.cc Makefile commit.h 
	mkdir -p $(BUILDDIR) 
	mkdir -p $(TBBBUILD) 
	$(CC) $(LDFLAGS) $(PRF_LDFLAGS) -c $< -o $@ -I. $(CPPFLAGS) $(PRF_CPPFLAGS)

$(BUILDDIR)%.o.PRF: plasticine/templates/%.cc Makefile commit.h 
	mkdir -p $(BUILDDIR) 
	mkdir -p $(TBBBUILD) 
	$(CC) $(LDFLAGS) $(PRF_LDFLAGS) -c $< -o $@ -I. $(CPPFLAGS) $(PRF_CPPFLAGS)

$(BUILDDIR)%.o.OPT: %.cc Makefile commit.h 
	mkdir -p $(BUILDDIR) 
	mkdir -p $(TBBBUILD) 
	$(CC) $(LDFLAGS) $(OPT_LDFLAGS) -c $< -o $@ -I. $(CPPFLAGS) $(OPT_CPPFLAGS)

$(BUILDDIR)%.o.OPT: plasticine/templates/%.cc Makefile commit.h 
	mkdir -p $(BUILDDIR) 
	mkdir -p $(TBBBUILD) 
	$(CC) $(LDFLAGS) $(OPT_LDFLAGS) -c $< -o $@ -I. $(CPPFLAGS) $(OPT_CPPFLAGS)

$(PLASTICINE_LIB_DBG): $(OBJS_NOMAIN_DBG) 
	mkdir -p $(LIBDIR)
	ld -r -o $(PLASTICINE_LIB_DBG) --whole-archive $(OBJS_NOMAIN_DBG)

$(PLASTICINE_LIB_PRF): $(OBJS_NOMAIN_PRF) 
	mkdir -p $(LIBDIR)
	ld -r -o $(PLASTICINE_LIB_PRF) --whole-archive $(OBJS_NOMAIN_PRF)

$(PLASTICINE_LIB_OPT): $(OBJS_NOMAIN_OPT) 
	mkdir -p $(LIBDIR)
	ld -r -o $(PLASTICINE_LIB_OPT) --whole-archive $(OBJS_NOMAIN_OPT)

clean:
	rm -f tungsten $(BUILDDIR)*
	cd tbb; make clean; cd -
	cd ramulator; make clean; cd -
	rm $(TBBBUILD)/*_release -rf
	rm -rf $(TESTOUT)

lint:
	cpplint --filter=-legal/copyright,-build/include_subdir,-build/c++11 --repository=. *.cc *.h tests/*.h tests/*.cc

-include build/*.d
