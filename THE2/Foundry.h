//
// Created by fatih on 21.04.2019.
//

#ifndef THE2NEW_FOUNDRY_H
#define THE2NEW_FOUNDRY_H

#include <semaphore.h>

extern "C" {
    #include "writeOutput.h"
}

#define TIMEOUT 5

class Foundry {
private:
    unsigned int id, interval, capacity, producedIngotCount, waitingIronCount, waitingCoalCount;
    pthread_t threadId;
    bool isActive;

    pthread_cond_t ironAndCoalReadyCV; // Condition variable

    pthread_mutex_t ironAndCoalCountMutex; // Mutex to protect waitingIronCount and waitingCoalCount
    // from being modified by multiple threads at the same time.

    sem_t ironStorageSlots, coalStorageSlots;


public:
    pthread_t getThreadId() const;
    Foundry(unsigned int id, unsigned int interval, unsigned int capacity);
    static void *foundry(void *args);
    void dropOre(OreType oreType);

};


#endif //THE2NEW_FOUNDRY_H
