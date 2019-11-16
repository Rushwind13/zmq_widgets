# loiosh
#HOMEDIR = /Users/jimbo
# work
#HOMEDIR = /Users/i859240
# stem
HOMEDIR = /Users/kristi
BASEDIR = $(HOMEDIR)/Documents/jimbo/Dev
OUTPUTDIR = ../bin
CC = g++
CC_FLAGS = -w -I/usr/local/include
LD_FLAGS = -L/usr/local/lib -L$(OUTPUTDIR) -lWidget -lzmq

EXEC = $(OUTPUTDIR)/$(notdir $(CURDIR:%/=%))
SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

$(EXEC): $(OBJECTS)
	$(CC) $(LD_FLAGS) $(OBJECTS) -o $(EXEC)

%.o: %.cpp
	$(CC) -c $(CC_FLAGS) $< -o $@

clean:
	rm -f $(EXEC) $(OBJECTS)
