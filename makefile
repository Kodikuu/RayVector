INCLUDES = \
	/Iincludes

OBJS = \
	src/main.obj \
	src/winlib.obj \
	src/audio.obj \
	src/kiss_fft130/kiss_fftr.obj \
	src/kiss_fft130/kiss_fft.obj \

BUILD = \
	main.obj \
	winlib.obj \
	audio.obj \
	kiss_fftr.obj \
	kiss_fft.obj \

LIBS = \
	libs/raylib.lib \
	libs/matoya.lib \
	Kernel32.lib \
	User32.lib \
	Gdi32.lib \
	Ole32.lib \
	OleAut32.lib \
	wbemuuid.lib \
	Advapi32.lib \
	Setupapi.lib \
	Ws2_32.lib \
	Shlwapi.lib \
	Winmm.lib \

FLAGS = \
	/W4 \
	/DWIN32_LEAN_AND_MEAN \
	/DUNICODE

CFLAGS = $(INCLUDES) $(FLAGS)
CPPFLAGS = $(INCLUDES) $(FLAGS)

all: $(OBJS)
	link $(BUILD) $(LIBS)
	del *.obj

run:
	.\main.exe 