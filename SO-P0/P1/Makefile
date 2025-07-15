#make : copila 
#make clean : limpia .outs
#make leaks : ejecuta valgrind

all: p1.out
		./p1.out


clean:
		rm -f *.out
leaks: p1.c
		gcc -Wall -g -O0 -o p1.out p1.c
		valgrind --leak-check=full --show-reachable=yes ./p1.out
p1.out: p1.c
		gcc -Wall -o p1.out p1.c
