output: main.o Miner.o Transporter.o Smelter.o Foundry.o writeOutput.o
	g++ main.o Miner.o Transporter.o Smelter.o Foundry.o writeOutput.o -o output -pthread

main.o: main.cpp
	g++ -c main.cpp

writeOutput.o: writeOutput.c writeOutput.h
	gcc -o writeOutput.o -c writeOutput.c

Miner.o: Miner.cpp Miner.h
	g++ -c Miner.cpp

Transporter.o: Transporter.cpp Transporter.h
	g++ -c Transporter.cpp

Smelter.o: Smelter.cpp Smelter.h
	g++ -c Smelter.cpp

Foundry.o: Foundry.cpp Foundry.h
	g++ -c Foundry.cpp

run:
	make
	./output
test:
	make
	./output < input.txt

clean:
	rm *.o output

