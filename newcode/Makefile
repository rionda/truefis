#
#  Copyright 2015 Matteo Riondato <matteo@cs.brown.edu>
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CPP=g++-mp-4.9
CPLEX=-I$(HOME)/Applications/IBM/ILOG/CPLEX_Studio1261/cplex/include -L$(HOME)/Applications/IBM/ILOG/CPLEX_Studio1261/cplex/lib/x86-64_osx/static_pic  -lilocplex -lcplex
CONCERT=-I$(HOME)/Applications/IBM/ILOG/CPLEX_Studio1261/concert/include -L$(HOME)/Applications/IBM/ILOG/CPLEX_Studio1261/concert/lib/x86-64_osx/static_pic  -lconcert
IGRAPH=-I/opt/local/include/ -L/opt/local/lib -ligraph
OPT=-Ofast -pipe -m64 -flto -funroll-loops
else
CPP=g++-4.9
CPLEX=-I/local/projects/cplex/cplex12.6/cplex/include -L/local/projects/cplex/cplex12.6/cplex/lib/x86-64_linux/static_pic -lilocplex -lcplex
CONCERT=-I/local/projects/cplex/cplex12.6/concert/include -L/local/projects/cplex/cplex12.6/concert/lib/x86-64_linux/static_pic -lconcert
IGRAPH=-I/home/matteo/include -L/home/matteo/lib -ligraph
#OPT=-Ofast -pipe -m64 -flto -march=native -mtune=native # not good for grid
OPT=-Ofast -pipe -m64 -flto -funroll-loops -mtune=generic -march=x86-64
endif
# Instruct CPLEX to compile with the standard headers, and without asserts and
# debugs
CPLEX_FLAGS=-fPIC -fexceptions -DIL_STD -DNDEBUG -DILO_LINUX

OTHER_LIBS=-lm -lpthread

#DEBUG=-DDEBUG
#PROFILE=-pg
#GDB=-ggdb3
#WARNS=-Wextra -Wall -Wstrict-aliasing=1 -Werror -pedantic-errors -Wshadow -Wno-sign-compare
WARNS=-Wall -Wstrict-aliasing=1 -Werror -pedantic-errors -Wno-sign-compare

ANSI_FLAGS=-ansi -std=c++11 #-fabi-version=6

CFLAGS=$(ANSI_FLAGS) $(WARNS) $(OPT)

.PHONY: all clean

all: compareFIs getTrueFIsBinom getTrueFIsHoldout getTrueFIsVCHoldout getTrueFIsVC sortFIs

archive: truefis.tar.bz2

clean:
	-/bin/rm -rf compareFIs getTrueFIsBinom getTrueFIsHoldout getTrueFIsVCHoldout getTrueFIsVC *.dSYM

compareFIs: compareFIs.cpp config.h dataset.cpp dataset.h itemsets.cpp itemsets.h Makefile
	$(CPP) dataset.cpp itemsets.cpp $@.cpp $(CFLAGS) -lm -o $@

compute_stats: compute_stats.cpp dataset.cpp dataset.h graph.cpp graph.h itemsets.cpp itemsets.h stats.cpp stats.h sukp.cpp sukp.h Makefile
	$(CPP) dataset.cpp graph.cpp itemsets.cpp stats.cpp sukp.cpp $@.cpp $(CFLAGS) $(CPLEX_FLAGS) $(CPLEX) $(CONCERT) $(IGRAPH) $(OTHER_LIBS) -o $@

getTrueFIsBinom: getTrueFIsBinom.cpp config.h dataset.cpp dataset.h itemsets.cpp itemsets.h pvalue.cpp pvalue.h Makefile
	$(CPP) dataset.cpp itemsets.cpp pvalue.cpp $@.cpp $(CFLAGS) -lm -o $@

getTrueFIsHoldout: getTrueFIsHoldout.cpp config.h dataset.cpp dataset.h itemsets.cpp itemsets.h pvalue.cpp pvalue.h Makefile
	$(CPP) dataset.cpp itemsets.cpp pvalue.cpp $@.cpp $(CFLAGS) -lm -o $@

getTrueFIsVC: getTrueFIsVC.cpp config.h dataset.cpp dataset.h epsilon.cpp epsilon.h graph.cpp graph.h itemsets.cpp itemsets.h stats.cpp stats.h sukp.cpp sukp.h Makefile
	$(CPP) dataset.cpp epsilon.cpp graph.cpp itemsets.cpp stats.cpp sukp.cpp $@.cpp $(CFLAGS) $(CPLEX_FLAGS) $(CPLEX) $(CONCERT) $(IGRAPH) $(OTHER_LIBS) -o $@

getTrueFIsVCHoldout: getTrueFIsVCHoldout.cpp config.h dataset.cpp dataset.h epsilon.cpp epsilon.h graph.cpp graph.h itemsets.cpp itemsets.h stats.cpp stats.h sukp.cpp sukp.h Makefile
	$(CPP) dataset.cpp epsilon.cpp graph.cpp itemsets.cpp stats.cpp sukp.cpp $@.cpp $(CFLAGS) $(CPLEX_FLAGS) $(CPLEX) $(CONCERT) $(IGRAPH) $(OTHER_LIBS) -o $@

sortFIs: sortFIs.cpp Makefile
	$(CPP) $@.cpp $(CFLAGS) -lm -o $@

truefis.tar.bz2: ../README.md ../LICENSE ../NOTICE Makefile *.h *.cpp
	-/bin/rm -rf truefis
	mkdir truefis
	cp ../README.md ../LICENSE ../NOTICE truefis
	cp Makefile truefis
	cp *.cpp *.h truefis
	env COPYFILE_DISABLE=1 tar cjvfh truefis.tar.bz2 truefis
	rm -rf truefis
