//
// Created by fatih on 21.04.2019.
//

#include <semaphore.h>
#include <cstdlib>
#include <zconf.h>
#include <iostream>
#include "Smelter.h"

#define TIMEOUT 5

Smelter::Smelter(unsigned int id, unsigned int interval, unsigned int capacity, OreType oreType) :
    id(id),
    interval(interval),
    capacity(capacity),
    oreType(oreType),
    threadId(-1),
    producedIngotCount(0),
    isActive(false),
    waitingOreCount(0),
    waitingOreCountMutex(PTHREAD_MUTEX_INITIALIZER) {

    pthread_cond_init(&twoOresReadyCV, nullptr);
    sem_init(&storageSlots, 0, capacity);
}

void *Smelter::smelter(void *args) {
    /* Thread routine for smelters. Takes a pointer to the smelter as the argument. */

    auto *smelter = (Smelter *) args;
    smelter->threadId = pthread_self();
    smelter->isActive = true;

    // Fill smelter info - created
    SmelterInfo smelterInfo = { smelter->id, smelter->oreType, smelter->capacity,
            smelter->waitingOreCount, smelter->producedIngotCount};
    WriteOutput(nullptr, nullptr, &smelterInfo, nullptr, SMELTER_CREATED);

    while (true) {

        // Quit if cannot produce ingots for a certain duration.
        // WaitUntilTwoOres() // or timeout after 5 seconds
        pthread_mutex_lock(&smelter->waitingOreCountMutex);
        if (smelter->waitingOreCount < 2) {
            timespec time = {0, 0};
            clock_gettime(CLOCK_REALTIME, &time);
            time.tv_sec += TIMEOUT;
            pthread_cond_timedwait(&smelter->twoOresReadyCV, &smelter->waitingOreCountMutex, &time);
            if (smelter->waitingOreCount < 2) { // Woke up because of timeout
                pthread_mutex_unlock(&smelter->waitingOreCountMutex);
                break; // If waitingOreCount is still less than 2, then timeout happened.
            }
            // Woke up because received signal
        }

        // Either waitingOreCound > 2 or signal received while sleeping.
        // Two ores are ready, produce an ingot
        smelter->waitingOreCount -= 2;
        pthread_mutex_unlock(&smelter->waitingOreCountMutex);

        // Update smelter info
        FillSmelterInfo(&smelterInfo, smelter->id, smelter->oreType, smelter->capacity,
                        smelter->waitingOreCount, smelter->producedIngotCount);
        // Notification: smelter started
        WriteOutput(nullptr, nullptr, &smelterInfo, nullptr, SMELTER_STARTED);


        // Sleep a value in range of Interval +- (Interval*0.01) microseconds for production
        usleep(smelter->interval - (smelter->interval*0.01) + (rand()%(int)(smelter->interval*0.02)));

        smelter->producedIngotCount++;

        // SmelterProduced()
        // Signal storageSlots twice to let transporter threads know 2 slots are available for
        // incoming ores.
        sem_post(&smelter->storageSlots);
        sem_post(&smelter->storageSlots);

        // Update smelter info
        FillSmelterInfo(&smelterInfo, smelter->id, smelter->oreType, smelter->capacity,
                        smelter->waitingOreCount, smelter->producedIngotCount);
        // Notification: smelter finished
        WriteOutput(nullptr, nullptr, &smelterInfo, nullptr, SMELTER_FINISHED);
    }


    smelter->isActive = false; // Smelter stopped

    // Update smelter info
    FillSmelterInfo(&smelterInfo, smelter->id, smelter->oreType, smelter->capacity,
                    smelter->waitingOreCount, smelter->producedIngotCount);
    // Notification: Smelter stopped
    WriteOutput(nullptr, nullptr, &smelterInfo, nullptr, SMELTER_STOPPED);


    pthread_exit(nullptr);
}

pthread_t Smelter::getThreadId() const {
    return threadId;
}

void Smelter::dropOre() {
    /* This function is called by transporters when they provide an ore to the smelter.
     * Function increments waitingOreCount, if waitingOreCount >= 2, signals the condition variable
     * twoOresReadyCV for the sleeping smelter thread to wake up.
     * */
    pthread_mutex_lock(&waitingOreCountMutex);
    if (++waitingOreCount >= 2) {
        pthread_cond_signal(&twoOresReadyCV);
    }
    pthread_mutex_unlock(&waitingOreCountMutex);
}
