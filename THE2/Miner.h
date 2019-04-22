//
// Created by fatih on 21.04.2019.
//

#ifndef THE2NEW_MINER_H
#define THE2NEW_MINER_H

#include <semaphore.h>

extern "C" {
    #include "writeOutput.h"
}


class Miner {
public:
    Miner(unsigned int id, unsigned int interval, unsigned int capacity, OreType oreType,
          unsigned int totalOre);
    ~Miner();

    pthread_t getThreadId();
    static void *miner(void *arg);


private:
    unsigned int id;
    unsigned int interval;
    unsigned int capacity;
    unsigned int totalOre;
    unsigned int currentOreCount;
    pthread_t threadId;
    OreType oreType;
    sem_t storageSlots; // Semaphore for the empty slots in the storage
    pthread_mutex_t oreCountMutex; // Mutex for the ore count
    sem_t producedOres; // Semaphore for the produced ores
    bool isActive;

};


#endif //THE2NEW_MINER_H
