CXX = g++.exe
LD = g++.exe
WINDRES = windres.exe

TARGET = fuchsi.exe

CFLAGS = -std=gnu++11 -Wall -O2
RESFLAGS = --input-format rc --output-format coff
LDFLAGS = -s -static -mwindows
LIB = -lgdi32 -luser32 -lkernel32 -lcomctl32 -lws2_32 -lshlwapi

INC = -I.
DIR_RELEASE = bin
OBJDIR_RELEASE = $(DIR_RELEASE)\\obj

OBJ_RELEASE = $(OBJDIR_RELEASE)\\ext\\happyhttp.o $(OBJDIR_RELEASE)\\utils\\http.o $(OBJDIR_RELEASE)\\resource.o $(OBJDIR_RELEASE)\\main.o

all: release

release: before_release $(OBJ_RELEASE)
	$(LD) $(LDFLAGS) $(INC) $(OBJ_RELEASE) $(LIB) -o $(DIR_RELEASE)\\$(TARGET)

before_release:
	if not exist bin md bin
	if not exist bin\obj md bin\obj
	if not exist bin\obj\utils md bin\obj\utils
	if not exist bin\obj\ext md bin\obj\ext

$(OBJDIR_RELEASE)\\main.o: main.cpp 
	$(CXX) $(CFLAGS) $(INC) -c main.cpp -o $(OBJDIR_RELEASE)\\main.o

$(OBJDIR_RELEASE)\\ext\\happyhttp.o: ext\\happyhttp.cpp ext\\happyhttp.h
	$(CXX) $(CFLAGS) $(INC) -c ext\\happyhttp.cpp -o $(OBJDIR_RELEASE)\\ext\\happyhttp.o

$(OBJDIR_RELEASE)\\utils\\http.o: utils\\http.cpp utils\\http.h
	$(CXX) $(CFLAGS) $(INC) -c utils\\http.cpp -o $(OBJDIR_RELEASE)\\utils\\http.o

$(OBJDIR_RELEASE)\\resource.o: resource.rc
	$(WINDRES) $(INC) --input resource.rc $(RESFLAGS) -o $(OBJDIR_RELEASE)\\resource.o

clean:
	rd /S /Q bin