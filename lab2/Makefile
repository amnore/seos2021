lab2: main.o my_print.o
	gcc -o lab2 -lreadline main.o my_print.o

main.o: main.c
	gcc -g -c main.c

my_print.o: my_print.asm
	nasm -g -f elf64 my_print.asm

clean:
	rm lab2 main.o my_print.o

.PHONY: clean
