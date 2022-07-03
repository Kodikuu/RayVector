INCLUDES = \
	/Iincludes

OBJS = \
	src/main.obj \
	src/winlib.obj \
	src/audio.obj \
	src/displayinfo.obj\
	src/kiss_fft130/kiss_fftr.obj \
	src/kiss_fft130/kiss_fft.obj \

BUILD = \
	main.obj \
	winlib.obj \
	audio.obj \
	displayinfo.obj\
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
	Shell32.lib \

FLAGS = \
	/W4 \
	/DWIN32_LEAN_AND_MEAN \
	/DUNICODE \
	/GL \
	/MD \

CFLAGS = $(INCLUDES) $(FLAGS)
CPPFLAGS = $(INCLUDES) $(FLAGS)

all: $(OBJS)
	link $(BUILD) $(LIBS) /LTCG /subsystem:console /NODEFAULTLIB:LIBCMT
	del *.obj

release: $(OBJS)
	link $(BUILD) $(LIBS) /LTCG /subsystem:windows /NODEFAULTLIB:LIBCMT
	del *.obj

run:
	.\main.exe
