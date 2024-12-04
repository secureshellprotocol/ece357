CC		:=	gcc
CFLAGS	:=	-g -Wall -Wextra -Wpedantic

all: shellgame frankensteins_sanity_check

frankensteins_sanity_check: sanity.o spinlock.o tas.o
	$(CC) $(CFLAGS) -o $@ $^

shellgame: spinlock.o tas.o sem.o shellgame.o
	$(CC) $(CFLAGS) -o $@ $^

sanity.o: sanity.c
	$(CC) $(CFLAGS) -o $@ -c $^

spinlock.o: spinlock.c
	$(CC) $(CFLAGS) -o $@ -c $^

tas.o: tas.S
	$(CC) $(CFLAGS) -o $@ -c $^

sem.o: sem.c
	$(CC) $(CFLAGS) -o $@ -c $^

shellgame.o: shellgame.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	rm *.o frankensteins_sanity_check shellgame
