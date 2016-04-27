CFLAGS= -g -Wall

main: main.o
	cc $(CFLAGS) -o main $<

main.o: main.c
	cc $(CFLAGS) -c main.c

wc: wc.o
	cc $(CFLAGS) -o wc $<

wc.o: wc.c
	cc $(CFLAGS) -c wc.c

map: map.o
	cc $(CFLAGS) -o map $<

map.o: map.c
	cc $(CFLAGS) -c map.c

.PHONY: clean
clean:
	rm -f main wc map main.o wc.o map.o
