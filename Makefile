CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -Isrc/include 
LDFLAGS = -lrt -pthread -lm

all: master player view

master:
	$(CC) $(CFLAGS) -o master src/master.c src/utils/master_functions.c src/utils/sh_memory.c $(LDFLAGS)

player:
	$(CC) $(CFLAGS) -o player src/player.c src/utils/player_functions.c src/utils/sh_memory.c $(LDFLAGS)

view:
	$(CC) $(CFLAGS) -o view src/view.c src/utils/view_functions.c src/utils/sh_memory.c $(LDFLAGS)

clean:
	rm -f master player view