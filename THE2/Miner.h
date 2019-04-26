//
// Created by fatih on 21.04.2019.
//

#ifndef MINER_H
#define MINER_H

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

    /* Called by transporters to pick up an ore if any available. Returns true if pickup
     * is successful, false otherwise. Synchronization issues are handled inside the function. */
    void pickUpOre();
    void signalStorageSpace();
    unsigned int getCurrentOreCount() const;
    bool isActive() const;
    unsigned int getId() const;
    unsigned int getCapacity() const;
    OreType getOreType() const;
    unsigned int getReservedOreCount() const;

    pthread_mutex_t oreCountMutex; // Mutex for the ore count
    void reserveOre();

private:
    unsigned int id;
    unsigned int interval;
    unsigned int capacity;
    unsigned int totalOre;
    unsigned int currentOreCount;
    pthread_t threadId;
    OreType oreType;
    bool active;
    sem_t storageSlots; // Semaphore for the empty slots in the storage
    unsigned int reservedOreCount;
    void writeMinerOutput(Action action);
};

#endif //MINER_H
