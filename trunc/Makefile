TARGET = oswanpsp
OBJS = src/apu.o src/fileio.o src/gpu.o src/main.o src/menu.o src/mon.o src/pg.o src/ws.o \
src/nec/nec.o src/debug/debug.o

INCDIR =
CFLAGS = -Wall -O3
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
BUILD_PRX = 1
PSP_FW_VERSION=352
LIBS= -lpsppower -lpspgu -lpspaudio

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Oswan PSP
PSP_EBOOT_ICON = icon.png

PSPSDK=$(shell psp-config --pspsdk-path)
ifndef PSPSDK
PSPSDK = /D/devkitPro/devkitPSP/psp/sdk
endif
include $(PSPSDK)/lib/build.mak

