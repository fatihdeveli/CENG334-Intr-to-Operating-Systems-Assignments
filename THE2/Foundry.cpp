//
// Created by fatih on 21.04.2019.
//

#include <cstdlib>
#include <zconf.h>
#include <iostream>
#include "Foundry.h"

extern sem_t producerSpacesForCoal;
extern sem_t producerSpacesForIron;

Foundry::Foundry(unsigned int id, unsigned int interval, unsigned int capacity) :
    id(id),
    interval(interval),
    capacity(capacity), // Capacity for iron and coal is the same
    threadId(-1),
    producedIngotCount(0),
    waitingIronCount(0),
    waitingCoalCount(0),
    active(false),
    ironAndCoalCountMutex(PTHREAD_MUTEX_INITIALIZER){

    pthread_cond_init(&ironAndCoalReadyCV, nullptr);

    pthread_create(&threadId, nullptr, foundry, this);

}

void *Foundry::foundry(void *args) {
    auto *foundry = (Foundry *) args;
    foundry->threadId = pthread_self();
    foundry->active = true;

    // Notification: foundry created
    foundry->writeFoundryOutput(FOUNDRY_CREATED);

    while (true) {
        // Quit if cannot produce ingots for a certain duration.
        // WaitForOneIronOneCoal() // or timeout after 5 seconds
        pthread_mutex_lock(&foundry->ironAndCoalCountMutex);
        if (foundry->waitingIronCount < 1 || foundry->waitingCoalCount < 1) {
            timespec time = {0, 0};
            clock_gettime(CLOCK_REALTIME, &time);
            time.tv_sec += TIMEOUT;
            pthread_cond_timedwait(&foundry->ironAndCoalReadyCV, &foundry->ironAndCoalCountMutex, &time);
            if (foundry->waitingIronCount < 1 || foundry->waitingCoalCount < 1) { // Woke up because of timeout
                pthread_mutex_unlock(&foundry->ironAndCoalCountMutex);
                break; // If waitingOreCount is still less than 2, then timeout happened.
            }
            // Woke up because received signal
        }
        // Either ores are enough or received signal while sleeping.
        // Two ores are ready, produce an ingot
        foundry->waitingIronCount--;
        foundry->waitingCoalCount--;
        pthread_mutex_unlock(&foundry->ironAndCoalCountMutex);

        // Notification: foundry started
        foundry->writeFoundryOutput(FOUNDRY_STARTED);

        // Sleep a value in range of Interval +- (Interval*0.01) microseconds for production
        usleep(foundry->interval - (foundry->interval*0.01) + (rand()%(int)(foundry->interval*0.02)));

        foundry->producedIngotCount++;

        // FoundryProduced()
        // Signal ironStorageSlots and coalStorageSlots to let transporter threads know about the
        // availability for incoming ores.
        sem_post(&producerSpacesForCoal);
        sem_post(&producerSpacesForIron);

        // Notification: foundry finished
        foundry->writeFoundryOutput(FOUNDRY_FINISHED);

    }

    foundry->active = false; // Foundry stopped

    // Notification: foundry stopped
    foundry->writeFoundryOutput(FOUNDRY_STOPPED);

    pthread_exit(nullptr);
}

void Foundry::dropOre(OreType &oreType) {
    /* This function is called by transporters when they provide an ore to the foundry.
     * Function increments waitingCoalCount or waitingIronCount, based on the incoming 
     * ore type. */
    pthread_mutex_lock(&ironAndCoalCountMutex);
    if (oreType == IRON) {
        waitingIronCount++;
    }
    else if (oreType == COAL) {
        waitingCoalCount++;
    }
    pthread_mutex_unlock(&ironAndCoalCountMutex);
}

void Foundry::signalDropOre() {
    /* This function is called by transporters after they provided an ore to the foundry.
     * If waitingCoalCount >= 1 AND waitingIronCount >= 1, signals the condition variable
     * ironAndCoalReadyCV for the sleeping foundry thread to wake up. */
    pthread_mutex_lock(&ironAndCoalCountMutex);
    if (waitingIronCount >= 1 && waitingCoalCount >= 1) {
        pthread_cond_signal(&ironAndCoalReadyCV);
    }
    pthread_mutex_unlock(&ironAndCoalCountMutex);
}

void Foundry::writeFoundryOutput(Action action) {
    FoundryInfo foundryInfo = {id, capacity, waitingIronCount, waitingCoalCount, producedIngotCount};
    WriteOutput(nullptr, nullptr, nullptr, &foundryInfo, action);
}

unsigned int Foundry::getId() const {
    return id;
}
unsigned int Foundry::getCapacity() const {
    return capacity;
}
unsigned int Foundry::getProducedIngotCount() const {
    return producedIngotCount;
}
unsigned int Foundry::getWaitingIronCount() const {
    return waitingIronCount;
}
unsigned int Foundry::getWaitingCoalCount() const {
    return waitingCoalCount;
}
pthread_t Foundry::getThreadId() const {
    return threadId;
}
bool Foundry::isActive() const {
    return active;
}


