CC = gcc
CFLAGS = -lpthread

output = main producer consumer

all: $(output)

main: main.c
	$(CC) main.c -o main $(CFLAGS)

producer: producer.c
	$(CC) producer.c -o producer $(CFLAGS)

consumer: consumer.c
	$(CC) consumer.c -o consumer $(CFLAGS)

clean:
	-rm $(output) *.log