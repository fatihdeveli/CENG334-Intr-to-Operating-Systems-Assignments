//
// Created by fatih on 21.04.2019.
//

#ifndef THE2NEW_SMELTER_H
#define THE2NEW_SMELTER_H

extern "C" {
#include "writeOutput.h"
}

class Smelter {
private:
    unsigned int id, interval, capacity, producedIngotCount, waitingOreCount;
    OreType oreType;
    pthread_t threadId;
    bool isActive;



    sem_t storageSlots; // Semaphore that is signaled by smelter when there is an available
    // slot in the storage for incoming ores. Transporters call wait to put ores in the storage.

    pthread_cond_t twoOresReadyCV; // Condition variable

    pthread_mutex_t waitingOreCountMutex; // Mutex to protect waitingOreCount from being
    // modified by multiple threads at the same time.

public:
    pthread_t getThreadId() const;
    void dropOre();

public:
    Smelter(unsigned int id, unsigned int interval, unsigned int capacity, OreType oreType);
    static void *smelter(void* args);
};


#endif //THE2NEW_SMELTER_H
