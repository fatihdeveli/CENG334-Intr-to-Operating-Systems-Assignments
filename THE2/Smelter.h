//
// Created by fatih on 21.04.2019.
//

#ifndef THE2NEW_SMELTER_H
#define THE2NEW_SMELTER_H

extern "C" {
#include "writeOutput.h"
}

class Smelter {

public:
    Smelter(unsigned int id, unsigned int interval, unsigned int capacity, OreType oreType);
    
    static void *smelter(void* args);

    // Called by transporters: Increments waitingOreCount (safely with lock)
    void dropOre();

    // Called by transporters: Signals the sleeping smelter thread if dropped ore count
    // has exceeded 2.
    void signalDropOre();

    unsigned int getId() const;
    unsigned int getWaitingOreCount() const;
    unsigned int getCapacity() const;
    unsigned int getProducedIngotCount() const;
    OreType getOreType() const;
    pthread_t getThreadId() const;
    bool isActive() const;
    
private:
    unsigned int id, interval, capacity, producedIngotCount, waitingOreCount;
    OreType oreType;
    pthread_t threadId;
    bool active;

    pthread_cond_t twoOresReadyCV; // Condition variable

    pthread_mutex_t waitingOreCountMutex; // Mutex to protect waitingOreCount from being
    // modified by multiple threads at the same time.
    void writeSmelterOutput(Action action);
};

#endif //THE2NEW_SMELTER_H
