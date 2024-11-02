CC 			 	:= gcc
OUTFILE_NAME 	:= shell

shell: shell.o tokenizer.o proc_runner.o utils.o
	$(CC) -g -o $(OUTFILE_NAME) shell.o tokenizer.o proc_runner.o utils.o

shell.o: shell.c shell.h
	$(CC) -g -c shell.c

tokenizer.o: tokenizer.c tokenizer.h
	$(CC) -g -c tokenizer.c

proc_runner.o: proc_runner.c proc_runner.h
	$(CC) -g -c proc_runner.c

utils.o: utils.c utils.h
	$(CC) -g -c utils.c

clean:
	rm $(OUTFILE_NAME) *.o
