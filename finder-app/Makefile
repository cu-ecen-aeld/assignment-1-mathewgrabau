# Makefile for writer project
# Builds the writer program (defined in the writer.c source file)
# Author: Mathew Grabau

OUTPUT=writer

${OUTPUT}: *.o
	${CROSS_COMPILE}gcc -o ${OUTPUT} ${OUTPUT}.o

*.o: *.c
	${CROSS_COMPILE}gcc -c *.c -o ${OUTPUT}.o
		

clean:
	rm -rf *.o ${OUTPUT}

