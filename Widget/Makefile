# loiosh
HOMEDIR = /Users/jimbo
# work
#HOMEDIR = /Users/i859240
BASEDIR = $(HOMEDIR)/Documents/code
OUTPUTDIR = ../bin
CC = g++
CC_FLAGS = -w
LD_FLAGS = -dynamiclib -lzmq

EXEC = $(OUTPUTDIR)/lib$(notdir $(CURDIR:%/=%)).dylib
SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

$(EXEC): $(OBJECTS)
	$(CC) $(LD_FLAGS) $(OBJECTS) -o $(EXEC)

%.o: %.cpp
	$(CC) -c $(CC_FLAGS) $< -o $@

clean:
	rm -f $(EXEC) $(OBJECTS)
