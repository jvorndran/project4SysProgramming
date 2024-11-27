CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = project5loader

all: $(TARGET)

$(TARGET): project5loader.o
	$(CC) $(CFLAGS) -o $(TARGET) project5loader.o

project5loader.o: project5loader.c
	$(CC) $(CFLAGS) -c project5loader.c

clean:
	rm -f *.o $(TARGET)