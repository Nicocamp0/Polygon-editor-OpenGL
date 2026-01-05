CC = gcc

CFLAGS = -Wall
CFLAGS_PPM = -Wall -Wno-unused-function

LFLAGS = -L/usr/lib -lm -lGL -lglut

EXEC = plot

OBJS = main.o Image.o Ppm.o

$(EXEC): $(OBJS)
	$(CC) $^ -o $@ $(LFLAGS)

main.o: main.c
	$(CC) -c $< -o $@ $(CFLAGS)

Image.o: Image.c
	$(CC) -c $< -o $@ $(CFLAGS)

Ppm.o: Ppm.c
	$(CC) -c $< -o $@ $(CFLAGS_PPM)

clean:
	/bin/rm -f $(EXEC) *.o
