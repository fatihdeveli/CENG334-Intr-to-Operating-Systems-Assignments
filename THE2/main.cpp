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


// Signaled whenever a producer finishes production of an ingot and releases
// storage space. Transporters call wait before they drop an ore to a producer.
sem_t producerSpacesForCopper;
sem_t producerSpacesForCoal;
sem_t producerSpacesForIron; 

int main() {
    InitWriteOutput();

    sem_init(&producedOres, 0, 0);
    sem_init(&producerSpacesForCopper, 0, 0);
    sem_init(&producerSpacesForCoal, 0, 0);
    sem_init(&producerSpacesForIron, 0, 0);
    

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
    for (int i = 0; i < Ns; i++) {
        unsigned int id, interval, capacity, type;
        OreType oreType;

        id = i+1;
        std::cin >> interval >> capacity >> type;
        oreType = intToOre(type);

        auto *smelter = new Smelter(id, interval, capacity, oreType);
        smelters.push_back(smelter);

        // Each smelter of type COPPER provides 2 storage spaces to drop copper ores
        if (oreType == COPPER) {
            sem_post(&producerSpacesForCopper);
            sem_post(&producerSpacesForCopper);
        } 
        // Each smelter of type IRON provides 2 storage spaces to drop iron ores
        else if (oreType == IRON) {
            sem_post(&producerSpacesForIron);
            sem_post(&producerSpacesForIron);
        }
    }

    //// Create foundry threads
    int Nf; // Number of foundries
    std::cin >> Nf;

    for (int i = 0; i < Nf; i++) {
        unsigned int id, interval, capacity;

        id = i+1;
        std::cin >> interval >> capacity;

        auto *foundry = new Foundry(id, interval, capacity);
        foundries.push_back(foundry);

        // Each foundry provides 1 storage space to drop iron ores and 1 storage space
        // to drop coal ores.
        sem_post(&producerSpacesForCoal);
        sem_post(&producerSpacesForIron);
    }

    //// Wait for threads to exit
    for (int i = 0; i < Nm; i++)
        pthread_join(miners[i]->getThreadId(), nullptr);
    for (int i = 0; i < Nt; i++)
        pthread_join(transporters[i]->getThreadId(), nullptr);
    for (int i = 0; i < Ns; i++)
        pthread_join(smelters[i]->getThreadId(), nullptr);
    for (int i = 0; i < Nf; i++)
        pthread_join(foundries[i]->getThreadId(), nullptr);

    for (int i = 0; i < Nm; i++)
        delete miners[i];
    for (int i = 0; i < Nt; i++)
        delete transporters[i];
    for (int i = 0; i < Ns; i++)
        delete smelters[i];
    for (int i = 0; i < Nf; i++)
        delete foundries[i];


    sem_destroy(&producedOres);
    sem_destroy(&producerSpacesForCopper);
    sem_destroy(&producerSpacesForCoal);
    sem_destroy(&producerSpacesForIron);

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
