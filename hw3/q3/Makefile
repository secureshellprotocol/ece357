CC 			 	:= gcc
OUTFILE_NAME 	:= shell

shell: shell.o tokenizer.o proc_runner.o
	$(CC) -g -o $(OUTFILE_NAME) shell.o tokenizer.o proc_runner.o

shell.o: shell.c macros.h tokenizer.h
	$(CC) -g -c shell.c

tokenizer.o: tokenizer.c macros.h tokenizer.h
	$(CC) -g -c tokenizer.c

proc_runner.o: proc_runner.c macros.h tokenizer.h proc_runner.h
	$(CC) -g -c proc_runner.c

clean:
	rm $(OUTFILE_NAME) *.o
