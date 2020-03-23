CC = gcc
CFLAGS = -g -Wall
PROG = SERVER

all: $(PROG)

$(PROG): $(PROG).c
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

run:
	./$(PROG) 5500

clean:
	rm -f *.o $(PROG)