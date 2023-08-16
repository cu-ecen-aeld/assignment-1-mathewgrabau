all: aesdsocket

OUTPUT=aesdsocket

aesdsocket.o: aesdsocket.c
	$(CC) $(CCFLAGS) $(INCLUDES) -c aesdsocket.c

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.o

clean:
	rm -rf *.o ${OUTPUT}
