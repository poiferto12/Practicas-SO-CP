#make : copila 
#make clean : limpia .outs
#make leaks : ejecuta valgrind

all: p3.out
		./p3.out


clean:
		rm -f *.out
leaks: p3.c
		gcc -Wall -g -O0 -o p3.out p3.c
		valgrind --leak-check=full --show-reachable=yes ./p3.out
p3.out: p3.c
		gcc -Wall -o p3.out p3.c
