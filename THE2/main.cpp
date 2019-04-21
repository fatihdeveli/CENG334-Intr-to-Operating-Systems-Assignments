#include <iostream>
#include <semaphore.h>
#include <vector>
#include <zconf.h>

#include "Miner.h"
#include "Transporter.h"
#include "Smelter.h"
#include "Foundry.h"




OreType intToOre(unsigned int i);

int main() {
    InitWriteOutput();

    //// Create miner threads
    int Nm; // Number of miners
    std::cin >> Nm;

    std::vector<Miner*> miners; // List of pointers to miners

    for (int i = 0; i < Nm; i++) {
        unsigned int id, interval, capacity, type, totalOre;
        OreType oreType;
        id = i+1; // ID's start from 1

        std::cin >> interval >> capacity >> type >> totalOre;
        oreType = intToOre(type);

        auto *miner = new Miner(id, interval, capacity, oreType, totalOre);
        miners.push_back(miner);

        pthread_t tid;
        pthread_create(&tid, nullptr, &Miner::miner, miner);

    }

    /// Create transporter threads
    int Nt; // Number of transporters
    std::cin >> Nt;

    std::vector<Transporter*> transporters;

    for (int i = 0; i < Nt; i++) {
        unsigned int id, time;
        id = i+1;
        std::cin >> time;

        auto *transporter = new Transporter(id, time);
        transporters.push_back(transporter);

        pthread_t tid;
        pthread_create(&tid, nullptr, Transporter::transporter, transporter);
    }

    /// Create smelter threads
    int Ns; // Number of smelters
    std::cin >> Ns;
    std::vector<Smelter*> smelters;
    for (int i = 0; i < Ns; i++) {
        unsigned int id, interval, capacity, type;
        OreType oreType;

        id = i+1;
        std::cin >> interval >> capacity >> type;
        oreType = intToOre(type);

        auto *smelter = new Smelter(id, interval, capacity, oreType);
        smelters.push_back(smelter);

        pthread_t tid;
        pthread_create(&tid, nullptr, Smelter::smelter, smelter);
    }

    /// Create foundry threads
    int Nf; // Number of foundries
    std::cin >> Nf;

    std::vector<Foundry*> foundries;

    for (int i = 0; i < Nf; i++) {
        unsigned int id, interval, capacity;

        id = i+1;
        std::cin >> interval >> capacity;

        auto *foundry = new Foundry(id, interval, capacity);
        foundries.push_back(foundry);

        pthread_t tid;
        pthread_create(&tid, nullptr, Foundry::foundry, foundry);
    }


    printf("Main thread is waiting for others to exit.\n");
    // Wait for threads to exit

    for (int i = 0; i < Nm; i++) {
        pthread_join(miners[i]->getThreadId(), nullptr);
        delete miners[i];
    }
    for (int i = 0; i < Nt; i++) {
        pthread_join(transporters[i]->getThreadId(), nullptr);
        delete transporters[i];
    }
    for (int i = 0; i < Ns; i++) {
        pthread_join(smelters[i]->getThreadId(), nullptr);
        delete smelters[i];
    }
    for (int i = 0; i < Nf; i++) {
        pthread_join(foundries[i]->getThreadId(), nullptr);
        delete foundries[i];
    }

    printf("Main thread exit\n");

    return 0;
}

OreType intToOre(unsigned int i) {
    switch (i) {
        case 0:
            return IRON;
        case 1:
            return COPPER;
        case 2:
            return COAL;
        default: // Normally code should never reach here
            printf("Error converting int to OreType.\n");
            return IRON;
    }
}