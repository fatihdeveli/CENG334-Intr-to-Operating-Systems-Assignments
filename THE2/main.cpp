#include <iostream>
#include <semaphore.h>
#include <vector>
#include <zconf.h>

#include "Miner.h"
#include "Transporter.h"
#include "Smelter.h"
#include "Foundry.h"


OreType intToOre(unsigned int i);

std::vector<Miner *> miners; // List of pointers to miners
std::vector<Smelter *> smelters; // List of pointers to smelters
std::vector<Foundry *> foundries; // List of pointers to foundries

sem_t producedOres; // Signaled whenever a miner produces an ore.
// Transporters call wait before they pick up an ore.

sem_t producerSpaces; // Signaled whenever a producer finishes production of an
// ingot and releases storage space. Transporters call wait before they drop an
// ore to a producer.

int main() {
    InitWriteOutput();

    sem_init(&producedOres, 0, 0);
    sem_init(&producerSpaces, 0, 0);

    //// Create miner threads
    int Nm; // Number of miners
    std::cin >> Nm;

    for (int i = 0; i < Nm; i++) {
        unsigned int id, interval, capacity, type, totalOre;
        OreType oreType;
        id = i+1; // ID's start from 1

        std::cin >> interval >> capacity >> type >> totalOre;
        oreType = intToOre(type);

        auto *miner = new Miner(id, interval, capacity, oreType, totalOre);
        miners.push_back(miner);

    }

    //// Create transporter threads
    int Nt; // Number of transporters
    std::cin >> Nt;

    std::vector<Transporter*> transporters;

    for (int i = 0; i < Nt; i++) {
        unsigned int id, time;
        id = i+1;
        std::cin >> time;

        auto *transporter = new Transporter(id, time);
        transporters.push_back(transporter);
    }

    //// Create smelter threads
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

        // Each smelter provides 2 storage spaces to drop ores
        sem_post(&producerSpaces);
        sem_post(&producerSpaces);
    }

    //// Create foundry threads
    int Nf; // Number of foundries
    std::cin >> Nf;

    std::vector<Foundry*> foundries;

    for (int i = 0; i < Nf; i++) {
        unsigned int id, interval, capacity;

        id = i+1;
        std::cin >> interval >> capacity;

        auto *foundry = new Foundry(id, interval, capacity);
        foundries.push_back(foundry);

        // Each foundry provides 2 storage spaces to drop ores
        sem_post(&producerSpaces);
        sem_post(&producerSpaces);
    }

    sleep(2);
    smelters[0]->dropOre();
    smelters[0]->dropOre();
    sleep(2);
    smelters[1]->dropOre();
    smelters[1]->dropOre();

    //// Wait for threads to exit
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

    sem_destroy(&producedOres);
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