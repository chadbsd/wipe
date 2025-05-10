CFLAGS = -O0 -g3 -pipe -Wall -Wextra

OBJ = wipe.o monocypher.o

wipe: ${OBJ}
	${CC} -o $@ ${OBJ} ${CFLAGS}
