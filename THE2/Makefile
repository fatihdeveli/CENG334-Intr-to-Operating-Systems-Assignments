all:
	gcc simulator.c writeOutput.c writeOutput.h -o simulator -pthread
run:
	gcc simulator.c writeOutput.c writeOutput.h -o simulator -pthread	
	./simulator
test:
	./simulator < sampleInput.txt
clean:
	rm -f *.o
