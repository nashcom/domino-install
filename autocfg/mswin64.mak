# makefile
# Nash!Com / Daniel Nashed
# Windows 64-bit version using
# Microsoft Visual Studio 2017

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

