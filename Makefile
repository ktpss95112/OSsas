all: main.c
	gcc -Wall -Wno-unused-result -O3 main.c -o main.out

clean:
	rm -f main.out