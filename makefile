# --- Platform ---
# http://stackoverflow.com/questions/714100/os-detecting-makefile
ifeq ($(OS),Windows_NT)
   echo "Use MSVC Solution/Project"
else
   UNAME = $(shell uname -s)

   ifeq ($(UNAME),Linux)
       TARGET=bin/hgr2rgb.elf
   endif
   ifeq ($(UNAME),Darwin)
        TARGET=bin/hgr2rgb.osx
   endif
endif

# --- Common ---

all: $(TARGET)

clean:
	@if [ -a $(TARGET) ] ; then rm $(TARGET); fi;

C_FLAGS=-Wall -Wextra -Isrc
DEP=src/main.cpp src/wsvideo.cpp src/wsvideo.h src/cs.cpp src/cs.h src/StdAfx.h

$(TARGET): $(DEP)
	g++ $(C_FLAGS) $< -o $@

