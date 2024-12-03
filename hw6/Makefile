CC		:=	gcc
CFLAGS	:=	-g -Wall -Wextra -Wpedantic

frankensteins_sanity_check.out: sanity.o spinlock.o tas.o
	$(CC) $(CFLAGS) -o $@ $^

sanity.o: sanity.c
	$(CC) $(CFLAGS) -o $@ -c $^

spinlock.o: spinlock.c
	$(CC) $(CFLAGS) -o $@ -c $^

tas.o: tas.S
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	rm *.o *.out
