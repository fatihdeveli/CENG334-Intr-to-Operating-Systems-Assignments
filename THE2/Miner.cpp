//
// Created by fatih on 21.04.2019.
//

#include <cstdlib>
#include <zconf.h>
#include "Miner.h"

Miner::Miner(unsigned int id, unsigned int interval, unsigned int capacity,
        OreType oreType, unsigned int totalOre) :
        id(id), interval(interval),
        capacity(capacity),
        oreType(oreType),
        totalOre(totalOre),
        currentOreCount(0),
        oreCountMutex(PTHREAD_MUTEX_INITIALIZER) {
    sem_init(&storageSlots, 0, capacity);
    sem_init(&producedOres, 0, 0);
    threadId = -1;
    isActive = false;
}




pthread_t Miner::getThreadId() {
    return threadId;
}

void *Miner::miner(void *arg) {
    auto* miner = (Miner*) arg;
    miner->threadId = pthread_self();
    miner->isActive = true;

    // Notification: Miner created
    MinerInfo minerInfo = {miner->id, miner->oreType, miner->capacity, miner->currentOreCount};
    WriteOutput(&minerInfo, nullptr, nullptr, nullptr, MINER_CREATED);

    while (miner->totalOre) { // While there are remaining ore in the mine
        // Wait until a storage space is cleared by a transporter and reserve a storage
        // space for the next ore.
        sem_wait(&miner->storageSlots);

        WriteOutput(&minerInfo, nullptr, nullptr, nullptr, MINER_STARTED);

        // Sleep a value in range of Interval +- (Interval*0.01) microseconds for production.
        usleep(miner->interval - (miner->interval*0.01) + (rand()%(int)(miner->interval*0.02)));

        // Produced an ore, increment currentOreCount.
        pthread_mutex_lock(&miner->oreCountMutex);
        miner->currentOreCount += 1;
        pthread_mutex_unlock(&miner->oreCountMutex);

        // Produced an ore, inform available transporters
        sem_post(&miner->producedOres);

        // Notification: Miner finished
        minerInfo.current_count = miner->currentOreCount; // Update miner info
        WriteOutput(&minerInfo, nullptr, nullptr, nullptr, MINER_FINISHED);

        // Sleep a value in range of Interval +- (Interval*0.01) microseconds for the next round.
        usleep(miner->interval - (miner->interval*0.01) + (rand()%(int)(miner->interval*0.02)));

        (miner->totalOre)--;
    } // Quit upon producing totalOre amount of ores.

    // Miner stopped
    miner->isActive = false;

    // Notification: Miner stopped
    WriteOutput(&minerInfo, nullptr, nullptr, nullptr, MINER_STOPPED);
    pthread_exit(nullptr);
}

Miner::~Miner() {
    sem_destroy(&storageSlots);
    sem_destroy(&producedOres);
}
