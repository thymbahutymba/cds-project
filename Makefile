CC = gcc
LDFLAGS = -pthread
CFLAGS = -Wall -Wextra -pedantic

all: manager client

manager: manager.o port.o
client: client.o port.o

clean:
	rm *.o
	rm manager client	