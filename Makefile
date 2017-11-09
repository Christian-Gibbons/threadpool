CC=gcc
RM=rm -rf
AR=ar
SRC := src
LIB := lib
INC := include
OBJ := objects

CFLAGS= -std=c11 -pthread -I./${INC} -I./queue/${INC}
LFLAGS= -pthread

.DEFAULT: ${LIB}/threadpool.a

.PHONY: clean all queue/${OBJ}/queue.o

${LIB}/threadpool.a: ${OBJ}/threadpool.o queue/${OBJ}/queue.o
	mkdir -p ${LIB}
	${AR} rcs $@ $^

${OBJ}/threadpool.o: ${SRC}/threadpool.c ${INC}/threadpool.h queue/${INC}/queue.h
	mkdir -p ${OBJ}
	${CC} -c ${CFLAGS} $< -o $@

queue/${OBJ}/queue.o:
	make -C queue ${OBJ}/queue.o

clean:
	${RM} ${LIB} ${OBJ}
	make -C queue clean

