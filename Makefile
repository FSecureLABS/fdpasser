CC ?= gcc
fdpasser: fdpasser.c
	$(CC) $(CFLAGS) -o $@ $^ 

clean:
	rm fdpasser
