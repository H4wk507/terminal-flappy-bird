main: main.c
	$(CC) main.c -o flappy -Wall -Wextra -pedantic -std=c99 -lncurses -lm -g
