CFLAGS = -O3 -march=native -pipe -Wall -Wextra

OBJ = wipe.o monocypher.o

wipe: ${OBJ}
	${CC} -o $@ ${OBJ} ${CFLAGS}
