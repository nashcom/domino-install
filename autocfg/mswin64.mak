###########################################################################
# Domino Auto Config (OneTouchConfig Tool)                                #
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

PROGRAM=autocfg
NODEBUG=1

# Link command

autocfg.exe: autocfg.obj cfg.obj
	link /SUBSYSTEM:CONSOLE autocfg.obj cfg.obj msvcrt.lib /PDB:$*.pdb /DEBUG /PDBSTRIPPED:$*_small.pdb /NODEFAULTLIB:LIBCMT -out:$@
	del $*.pdb $*.sym
	rename $*_small.pdb $*.pdb

# Compile command


cfg.obj: cfg.cpp
	cl -nologo -c -D_MT -MT /Zi /Ot /O2 /Ob2 /Oy- -Gd /Gy /GF /Gs4096 /GS- /favor:INTEL64 /EHsc /Zc:wchar_t- -Zl -W1 -DNT -DW32 -DW -DW64 -DND64 -D_AMD64_ -DDTRACE -D_CRT_SECURE_NO_WARNINGS -DPRODUCTION_VERSION  cfg.cpp

autocfg.obj: autocfg.cpp
	cl -nologo -c -D_MT -MT /Zi /Ot /O2 /Ob2 /Oy- -Gd /Gy /GF /Gs4096 /GS- /favor:INTEL64 /EHsc /Zc:wchar_t- -Zl -W1 -DNT -DW32 -DW -DW64 -DND64 -D_AMD64_ -DDTRACE -D_CRT_SECURE_NO_WARNINGS -DPRODUCTION_VERSION  autocfg.cpp

all: autocfg.exe

clean:
	del *.obj *.pdb *.exe *.dll *.ilk *.sym *.map

