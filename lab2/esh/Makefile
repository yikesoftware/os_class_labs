SHELL = /bin/sh
OBJS = main.o built-in.o
CFLAGS = -Wall -O2 --static
CC = gcc

esh:${OBJS}
	${CC} ${CFLAGS} -o esh ${OBJS}
    
main.o:main.c internal.h
	${CC} ${CFLAGS} -c -o main.o main.c

built-in.o:built-in.c internal.h
	${CC} ${CFLAGS} -c -o built-in.o built-in.c
    
.PHONY:clean
clean:
	rm -f ${OBJS} *.core
    
install: esh
	sudo cp esh /bin/esh
