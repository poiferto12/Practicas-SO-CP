#make : copila 
#make clean : limpia .outs
#make leaks : ejecuta valgrind

all: p2.out
		./p2.out


clean:
		rm -f *.out
leaks: p2.c
		gcc -Wall -g -O0 -o p2.out p2.c
		valgrind --leak-check=full --show-reachable=yes ./p2.out
p2.out: p2.c
		gcc -Wall -o p2.out p2.c