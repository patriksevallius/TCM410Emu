
emulator: emulator.so main.o
	gcc -Wall -g -o emulator emulator.o main.o

emulator.so: emulator.o
	gcc -shared -o emulator.so emulator.o

emulator.o: emulator.c emulator.h opcode.h
	gcc -Wall -g -fPIC -o emulator.o -c emulator.c

main.o: main.c
	gcc -Wall -g -o main.o -c main.c
