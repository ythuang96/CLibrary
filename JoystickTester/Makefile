# This is a general use makefile for projects written in C.
# Just change the target name to match your main source code filename.
TARGET = joysticktester

# Path for the C Library functions needs to be set with the environment variables:
# Add the line:
# export CPATH=/home/pi/CLibrary:$CPATH
# export LIBRARY_PATH=/home/pi/CLibrary:$LIBRARY_PATH
# to ~/.bashrc

# Path to the header files so that the full path does not need to be specified
# for the include statement
INCLUDEPATH = -I ./ -I ../

# Path to search for source files, separated wwith :
VPATH = ./

SOURCES		:= $(wildcard ./*.c)
INCLUDES	:=




CC		:= gcc
LINKER		:= gcc
CFLAGS		:= -c -g -Wall -Wstrict-prototypes -ansi -pedantic -O3 -std=c99
LFLAGS		:= -lmysdl -lmyclib -pthread -lm -lrt -lcurl `sdl2-config --cflags --libs`


# replace .c with .o
# then remove the directory so that all .o files are generated in current dir
OBJECTS		:= $(notdir  $(patsubst %.c, %.o,$(SOURCES)) )

prefix		:= /usr/local
RM		:= rm -f
INSTALL		:= install -m 4755
INSTALLDIR	:= install -d -m 755


# linking Objects
$(TARGET): $(OBJECTS) $(INCLUDES)
	@$(LINKER) $(INCLUDEPATH) -o $@ $(OBJECTS) $(LFLAGS)
	@echo "Made: $@"

# compiling command
$(OBJECTS): %.o : %.c $(INCLUDES)
	@$(CC) $(CFLAGS) $(INCLUDEPATH) $< -o $@ $(LFLAGS)
	@echo "Compiled: $@"

all:	$(TARGET)

test: $(TARGET)
	@./$(TARGET)

install:
	@$(MAKE) --no-print-directory
	@$(INSTALLDIR) $(DESTDIR)$(prefix)/bin
	@$(INSTALL) $(TARGET) $(DESTDIR)$(prefix)/bin
	@echo "$(TARGET) Install Complete"

clean:
	@$(RM) $(OBJECTS)
	@$(RM) $(TARGET)
	@echo "$(TARGET) Clean Complete"

uninstall:
	@$(RM) $(DESTDIR)$(prefix)/bin/$(TARGET)
	@echo "$(TARGET) Uninstall Complete"

run: $(TARGET)
	@./$(TARGET)



