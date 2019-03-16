all: mapreduce.o
	gcc -o mapreduce mapreduce.o

mapreduce.o: mapreduce.c
	gcc -c mapreduce.c

clean:
	rm -f *.o
