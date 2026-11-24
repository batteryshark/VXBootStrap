# Cross-builds both bootstraps with MinGW-w64.
# The output names matter: vxlauncher and pdx look for exactly
# VXBootstrap32.exe and VXBootstrap64.exe.

BIN    ?= bin
SRC     = src/vxbootstrap.cpp
DEPS    = $(SRC) src/strutils.hpp
CFLAGS  = -fshort-wchar -mwindows
LDFLAGS = -static -static-libgcc -lstdc++

all: $(BIN)/VXBootstrap64.exe $(BIN)/VXBootstrap32.exe

$(BIN)/VXBootstrap64.exe: $(DEPS)
	mkdir -p $(BIN)
	x86_64-w64-mingw32-gcc $(CFLAGS) $(SRC) $(LDFLAGS) -o $@

$(BIN)/VXBootstrap32.exe: $(DEPS)
	mkdir -p $(BIN)
	i686-w64-mingw32-gcc $(CFLAGS) $(SRC) $(LDFLAGS) -o $@

clean:
	rm -rf $(BIN)

.PHONY: all clean
