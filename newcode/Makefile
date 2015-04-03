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
IGRAPH=-I/opt/local/include/ -L/opt/local/lib
else
CPP=g++-4.9
CPLEX=-I/local/projects/cplex/current/cplex/x86_64/include -L/local/projects/cplex/current/cplex/x86_64/lib
IGRAPH=-I/home/matteo/include -L/home/matteo/lib
endif
#DEBUG=-DDEBUG

OPT=-Ofast -pipe -m64 -march=native -mtune=native
#PROFILE=-pg
#GDB=-ggdb3

# For some reasons, -static does not work on Mac OS X (MacPorts g++ 4.9) (FIXME)
STATIC=-static
ifeq ($(UNAME), Darwin)
STATIC=-static-libgcc -static-libstdc++
endif

WARNS=-Wextra -Wall -Wstrict-aliasing=1 -Werror -pedantic-errors -Wshadow -Wno-sign-compare

ANSI_FLAGS=-ansi -std=c++11 -fabi-version=6

CFLAGS=$(ANSI_FLAGS) $(WARNS) $(OPT) $(STATIC) $(GDB) $(DEBUG) $(PROFILE)

.PHONY: all clean

all: compute_stats

archive: truefis.tar.bz2

clean:
	-/bin/rm -rf compute_stats *.dSYM

compute_stats: compute_stats.cpp dataset.cpp dataset.h graph.cpp graph.h stats.cpp stats.h sukp.cpp sukp.h Makefile
	$(CPP) dataset.cpp graph.cpp stats.cpp sukp.cpp $@.cpp $(CFLAGS) -o $@ $(IGRAPH) -ligraph -lilocplex -lconcert -lcplex -lm -lpthread

truefis.tar.bz2: ../README.md ../LICENSE ../NOTICE Makefile *.h *.cpp
	-/bin/rm -rf truefis
	mkdir truefis
	cp ../README.md ../LICENSE ../NOTICE truefis
	cp Makefile truefis
	cp *.cpp *.h truefis
	env COPYFILE_DISABLE=1 tar cjvfh truefis.tar.bz2 truefis
	rm -rf truefis
