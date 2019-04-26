//
// Created by fatih on 21.04.2019.
//

#ifndef FOUNDRY_H
#define FOUNDRY_H

#include <semaphore.h>

extern "C" {
    #include "writeOutput.h"
}

#define TIMEOUT 5

class Foundry {
public:
    Foundry(unsigned int id, unsigned int interval, unsigned int capacity);

    static void *foundry(void *args); // Foundry thread routine
    void dropOre(OreType &oreType);
    void signalDropOre();

    unsigned int getId() const;
    unsigned int getCapacity() const;
    unsigned int getProducedIngotCount() const;
    unsigned int getWaitingIronCount() const;
    unsigned int getWaitingCoalCount() const;
    pthread_t getThreadId() const;
    bool isActive() const;


private:
    unsigned int id, interval, capacity, producedIngotCount, waitingIronCount, waitingCoalCount;
    pthread_t threadId;
    bool active;

    pthread_cond_t ironAndCoalReadyCV; // Condition variable to check the status of incoming ores.

    pthread_mutex_t ironAndCoalCountMutex; // Mutex to protect waitingIronCount and waitingCoalCount
    // from being modified by multiple threads at the same time.

    void writeFoundryOutput(Action action);
};


#endif //FOUNDRY_H
