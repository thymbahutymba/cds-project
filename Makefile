CC = gcc
LDFLAGS = -pthread -lrt
CFLAGS = -Wall -Wextra -pedantic
OBJECTS = manager.o client.o port.o

all: manager client

manager: manager.o port.o
client: client.o port.o


clean:
	rm *.o
	rm manager client	