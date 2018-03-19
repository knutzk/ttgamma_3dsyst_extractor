CC := c++

LIBS = `root-config --libs`
CFLAGS = `root-config --cflags`
MISCFLAGS = -std=c++1y

SRC = $(wildcard ./*.cc)
OBJ = $(SRC:.cc=.o)
TARGET = derive-syst.exe

.PHONY: all

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(MISCFLAGS) $(LIBS) -o $@ $^

%.o: %.cc
	$(CC) $(CFLAGS) $(MISCFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	$(RM) $(OBJ)
	$(RM) $(TARGET)
