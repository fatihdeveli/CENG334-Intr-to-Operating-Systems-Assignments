//
// Created by fatih on 21.04.2019.
//

#include <semaphore.h>
#include <cstdlib>
#include <zconf.h>
#include <iostream>
#include "Smelter.h"

#define TIMEOUT 5

extern sem_t producerSpacesForCopper;
extern sem_t producerSpacesForIron;

Smelter::Smelter(unsigned int id, unsigned int interval, unsigned int capacity, OreType oreType) :
    id(id),
    interval(interval),
    capacity(capacity),
    oreType(oreType),
    threadId(-1),
    producedIngotCount(0),
    active(false),
    waitingOreCount(0),
    waitingOreCountMutex(PTHREAD_MUTEX_INITIALIZER) {

    pthread_cond_init(&twoOresReadyCV, nullptr);

    pthread_create(&threadId, nullptr, smelter, this);
}

void *Smelter::smelter(void *args) {
    /* Thread routine for smelters. Takes a pointer to the smelter as the argument. */

    auto *smelter = (Smelter *) args;
    smelter->threadId = pthread_self();
    smelter->active = true;

    // Notification: smelter created
    smelter->writeSmelterOutput(SMELTER_CREATED);

    while (true) {
        // Wait until two ores are dropped. Quit if cannot produce ingots for 5 seconds.
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
        // Either waitingOreCount > 2 or signal received while sleeping.
        // Two ores are ready, produce an ingot
        smelter->waitingOreCount -= 2;
        pthread_mutex_unlock(&smelter->waitingOreCountMutex);

        // Notification: smelter started
        smelter->writeSmelterOutput(SMELTER_STARTED);

        // Sleep a value in range of Interval +- (Interval*0.01) microseconds for production
        usleep(smelter->interval - (smelter->interval*0.01) + (rand()%(int)(smelter->interval*0.02)));

        smelter->producedIngotCount++;

        // Smelter produced: Signal to let transporter threads know 2 slots are available for incoming ores.
        if (smelter->oreType == COPPER) {
            sem_post(&producerSpacesForCopper);
            sem_post(&producerSpacesForCopper);
        }
        else if (smelter->oreType == IRON) {
            sem_post(&producerSpacesForIron);
            sem_post(&producerSpacesForIron);
        }

        // Notification: smelter finished
        smelter->writeSmelterOutput(SMELTER_FINISHED);
    }

    smelter->active = false; // Smelter stopped

    // Notification: Smelter stopped
    smelter->writeSmelterOutput(SMELTER_STOPPED);

    pthread_exit(nullptr);
}

void Smelter::writeSmelterOutput(Action action) {
    SmelterInfo smelterInfo {id, oreType, capacity, waitingOreCount, producedIngotCount};
    WriteOutput(nullptr, nullptr, &smelterInfo, nullptr, action);
}


void Smelter::dropOre() {
    /* This function is called by transporters when they provide an ore to the smelter.
     * Function increments waitingOreCount */
    pthread_mutex_lock(&waitingOreCountMutex);
    waitingOreCount++;
    pthread_mutex_unlock(&waitingOreCountMutex);
}

void Smelter::signalDropOre() {
    /* This function is called by transporters when they provide an ore to the smelter.
     * If waitingOreCount >= 2, signals the condition variable twoOresReadyCV for the 
     * sleeping smelter thread to wake up. */
    pthread_mutex_lock(&waitingOreCountMutex);
    if (waitingOreCount >= 2) {
        pthread_cond_signal(&twoOresReadyCV);
    }
    pthread_mutex_unlock(&waitingOreCountMutex);
}


unsigned int Smelter::getId() const{
    return id;
}

unsigned int Smelter::getWaitingOreCount() const {
    return waitingOreCount;
}

unsigned int Smelter::getCapacity() const {
    return capacity;
}

unsigned int Smelter::getProducedIngotCount() const {
    return producedIngotCount;
}

OreType Smelter::getOreType() const {
    return oreType;
}

pthread_t Smelter::getThreadId() const {
    return threadId;
}

bool Smelter::isActive() const {
    return active;
}