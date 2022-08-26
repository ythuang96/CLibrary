# This compiles a library, the target must be in .a fromat
TARGET = ../libmysdl.a

# Path to the header files so that the full path does not need to be specified
# for the include statement
INCLUDEPATH = -I ../

# Path to search for source files, separated wwith :
VPATH = ../src

SOURCES		:= $(wildcard ../src/SDL_*.c)
INCLUDES	:= ../MySDL.h


CC		:= gcc
CFLAGS		:= -c -g -Wall -Wstrict-prototypes -ansi -pedantic -O3 -std=c99
LFLAGS		:= -lcurl -pthread -lm -lrt `sdl2-config --cflags --libs`

# replace .c with .o
# then remove the directory so that all .o files are generated in current dir
OBJECTS		:= $(notdir  $(patsubst %.c, %.o,$(SOURCES)) )


# linking Objects into .a library file
$(TARGET): $(OBJECTS)
	@ar rcs $(TARGET) $(OBJECTS)
	@echo "Made: $@"

# compiling command
$(OBJECTS): %.o : %.c $(INCLUDES)
	@$(CC) $(CFLAGS) $(INCLUDEPATH) $< -o $@ $(LFLAGS)
	@echo "Compiled: $@"

all:	$(TARGET)

clean:
	@$(RM) $(OBJECTS)
	@$(RM) $(TARGET)
	@echo "$(TARGET) Clean Complete"




