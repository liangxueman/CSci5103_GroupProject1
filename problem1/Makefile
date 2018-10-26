CC = gcc
CFLAGS = -lpthread

output = prodcons
files = prodcons.c

all: $(output)

$(output): $(files)
	$(CC) $(files) -o $(output) $(CFLAGS)

clean:
	-rm $(output) *.log