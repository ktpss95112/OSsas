all: main.c
	gcc -Wall main.c -o main.out

clean:
	rm -f main.out