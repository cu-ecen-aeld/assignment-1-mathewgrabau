# Makefile for writer project
# Builds the writer program (defined in the writer.c source file)
# Author: Mathew Grabau

all: writer

OUTPUT=writer

writer.o: writer.c
	$(CC) $(CCFLAGS) $(INCLUDES) -c writer.c

writer: writer.o
	$(CC) $(LDFLAGS) -o writer

clean:
	rm -rf *.o ${OUTPUT}

