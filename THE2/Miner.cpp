//
// Created by fatih on 21.04.2019.
//

#include <cstdlib>
#include <zconf.h>
#include "Miner.h"

extern sem_t producedOres;

Miner::Miner(unsigned int id, unsigned int interval, unsigned int capacity,
        OreType oreType, unsigned int totalOre) :
        id(id),
        interval(interval),
        capacity(capacity),
        oreType(oreType),
        totalOre(totalOre),
        threadId(-1),
        currentOreCount(0),
        reservedOreCount(0),
        active(false),
        oreCountMutex(PTHREAD_MUTEX_INITIALIZER) {

    sem_init(&storageSlots, 0, capacity);

    //
    pthread_create(&threadId, nullptr, miner, this);

}


pthread_t Miner::getThreadId() {
    return threadId;
}

void *Miner::miner(void *arg) {
    auto* miner = (Miner*) arg;
    miner->threadId = pthread_self();
    miner->active = true;

    // Notification: Miner created
    miner->writeMinerOutput(MINER_CREATED);

    while (miner->totalOre) { // While there are remaining ore in the mine
        // Wait until a storage space is cleared by a transporter and reserve a storage
        // space for the next ore.
        sem_wait(&miner->storageSlots);

        miner->writeMinerOutput(MINER_STARTED);

        // Sleep a value in range of Interval +- (Interval*0.01) microseconds for production.
        usleep(miner->interval - (miner->interval*0.01) + (rand()%(int)(miner->interval*0.02)));

        // Produced an ore, increment currentOreCount, inform available transporters
        pthread_mutex_lock(&miner->oreCountMutex);
        miner->currentOreCount += 1;
        pthread_mutex_unlock(&miner->oreCountMutex);
        sem_post(&producedOres);

        if(--(miner->totalOre) == 0) // Miner will stop
            miner->active = false;

        // Notification: Miner finished
        miner->writeMinerOutput(MINER_FINISHED);

        // Sleep a value in range of Interval +- (Interval*0.01) microseconds for the next round.
        usleep(miner->interval - (miner->interval*0.01) + (rand()%(int)(miner->interval*0.02)));

    } // Quit upon producing totalOre amount of ores.

    // Notification: Miner stopped
    miner->writeMinerOutput(MINER_STOPPED);

    pthread_exit(nullptr);
}

Miner::~Miner() {
    sem_destroy(&storageSlots);
}

bool Miner::isActive() const {
    return active;
}

unsigned int Miner::getCurrentOreCount() const {
    return currentOreCount;
}

void Miner::pickUpOre() {
    pthread_mutex_lock(&oreCountMutex);
    currentOreCount--;
    reservedOreCount--;
    pthread_mutex_unlock(&oreCountMutex);
}

void Miner::signalStorageSpace() {
    sem_post(&storageSlots);
}

void Miner::writeMinerOutput(Action action) {
    MinerInfo minerInfo = {id, oreType, capacity, currentOreCount};
    WriteOutput(&minerInfo, nullptr, nullptr, nullptr, action);
}

void Miner::reserveOre() {
    reservedOreCount++;
}

OreType Miner::getOreType() const {
    return oreType;
}

unsigned int Miner::getId() const {
    return id;
}

unsigned int Miner::getCapacity() const {
    return capacity;
}

unsigned int Miner::getReservedOreCount() const {
    return reservedOreCount;
}
