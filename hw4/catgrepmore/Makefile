CC 			 	:= gcc
OUTFILE_NAME 	:= cgm

cgm: main.o pipeline.o
	$(CC) -g -o $(OUTFILE_NAME) main.o pipeline.o

main.o: main.c
	$(CC) -g -c main.c

pipeline.o: pipeline.c pipeline.h
	$(CC) -g -c pipeline.c

clean:
	rm $(OUTFILE_NAME) *.o
