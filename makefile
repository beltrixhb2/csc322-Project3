all: your_program

your_program: main.c
	gcc main.c -lm

clean:
	rm -f a.out 
