COMPILER=G++

# todo: object files into output path, processing c / c++ files in the same time (?), nested directories for source files (?)
C = c
OUTPUT_PATH = bin/
SOURCE_PATH = src/
EXE = bin/mbpfan

ifeq ($(COMPILER), G++)
  ifeq ($(OS),Windows_NT)
    OBJ = obj
  else
    OBJ = o
  endif
  COPT = 
  CCMD = g++
  OBJFLAG = -o
  EXEFLAG = -o
# INCLUDES = -I../.includes
  INCLUDES =
# LIBS = -lgc
  LIBS = -lm
# LIBPATH = -L../gc/.libs
  LIBPATH =
  CPPFLAGS = $(COPT) -g $(INCLUDES)
  LDFLAGS = $(LIBPATH) -g $(LIBS)
  DEP = dep
else
  OBJ = obj
  COPT = /O2
  CCMD = cl
  OBJFLAG = /Fo
  EXEFLAG = /Fe
# INCLUDES = /I..\\.includes
  INCLUDES =
# LIBS = ..\\.libs\\libgc.lib
  LIBS =
  CPPFLAGS = $(COPT) /DEBUG $(INCLUDES)
  LDFLAGS = /DEBUG
endif

OBJS := $(patsubst %.$(C),%.$(OBJ),$(wildcard $(SOURCE_PATH)*.$(C)))

%.$(OBJ):%.$(C)
	mkdir -p bin
	@echo Compiling $(basename $<)...
	$(CCMD) -c $(CPPFLAGS) $(CXXFLAGS) $< $(OBJFLAG)$@

all: $(OBJS)
	@echo Linking...
	$(CCMD) $(LDFLAGS) $^ $(LIBS) $(EXEFLAG) $(EXE)

clean:
	rm -rf $(SOURCE_PATH)*.$(OBJ) $(EXE)

install:
	cp $(EXE) /usr/sbin
	@echo "-----------------------------------------------------------------------------"
	@echo "An init file suitable for /lib/lsb/init-functions (Debian) is located in"
	@echo "in the main folder of the source files. It is called mbpfan.init.debian"
	@echo "Rename it to mbpfan, give it execution permissions (chmod +x mbpfan)"
	@echo "and move it to /etc/init.d"
	@echo "Then, add it to the default runlevels with sudo update-rc.d mbpfan defaults"
	@echo ""
	@echo "Additionally, an init file suitable for /etc/rc.d/init.d/functions"
	@echo "(RHEL/CentOS & Fedora) is also located at the same place, this file is called"
	@echo "mbpfan.init.redhat. Also rename it to mbpfan, give it execution permissions"
	@echo "and move it to /etc/init.d"
	@echo "To add the script to the default runlevels, run the following as root:"
	@echo "chkconfig --level 2345 mbpfan on && chkconfig --level 016 mbpfan off"
	@echo ""
	@echo "For upstart based init systems (Ubuntu), an example upstart job has been"
	@echo "provided for use in place of the LSB-style init script. To use, execute"
	@echo "as root:"
	@echo "cp mbpfan.upstart /etc/init/mbpfan.conf"
	@echo "start mbpfan"
	@echo "-----------------------------------------------------------------------------"

rebuild: clean all
#rebuild is not entirely correct
