###########################################################################
# Domino Auto Config (OneTouchConfig Tool)                                #
# Version 0.1 02.02.2023                                                  #
# (C) Copyright Daniel Nashed/NashCom 2023                                #
#                                                                         #
# Licensed under the Apache License, Version 2.0 (the "License");         #
# you may not use this file except in compliance with the License.        #
# You may obtain a copy of the License at                                 #
#                                                                         #
#      http://www.apache.org/licenses/LICENSE-2.0                         #
#                                                                         #
# Unless required by applicable law or agreed to in writing, software     #
# distributed under the License is distributed on an "AS IS" BASIS,       #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.#
# See the License for the specific language governing permissions and     #
# limitations under the License.                                          #
###########################################################################

CC=g++
CFLAGS= -g -Wall -c -m64 -fPIC -fpermissive -Wno-write-strings 

PROGRAM=autocfg

all: autocfg

autocfg: autocfg.o cfg.o
	$(CC) autocfg.o cfg.o -o bin/linux64/$@

cfg.o: cfg.cpp
	$(CC) $(CFLAGS) cfg.cpp

autocfg.o: autocfg.cpp
	$(CC) $(CFLAGS) autocfg.cpp


clean:
	rm -f  *.o bin/linux64/autocfg

test: all
	./$(PROGRAM)
