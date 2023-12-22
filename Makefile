
OBJECTS=main.o z80.o mem.o tms9918.o sk1100.o console.o gui.o
CFLAGS=-Wall -Wextra -DDISABLE_Z80_TRACE
LDFLAGS=-lncurses -lSDL2

all: sgtouzen

sgtouzen: ${OBJECTS}
	gcc -o sgtouzen $^ ${LDFLAGS}

main.o: main.c
	gcc -c $^ ${CFLAGS}

z80.o: z80.c
	gcc -c $^ ${CFLAGS}

mem.o: mem.c
	gcc -c $^ ${CFLAGS}

tms9918.o: tms9918.c
	gcc -c $^ ${CFLAGS}

sk1100.o: sk1100.c
	gcc -c $^ ${CFLAGS}

console.o: console.c
	gcc -c $^ ${CFLAGS}

gui.o: gui.c
	gcc -c $^ ${CFLAGS}

.PHONY: clean
clean:
	rm -f *.o sgtouzen

