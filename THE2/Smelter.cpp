//
// Created by fatih on 21.04.2019.
//

#include "Smelter.h"

Smelter::Smelter(unsigned int id, unsigned int interval, unsigned int capacity, OreType oreType)
    : id(id), interval(interval), capacity(capacity), oreType(oreType), threadId(-1),
    producedIngotCount(0) {

}

void *Smelter::smelter(void *args) {
    auto *smelter = (Smelter *) args;
    smelter->threadId = pthread_self();


    pthread_exit(nullptr);
}

pthread_t Smelter::getThreadId() const {
    return threadId;
}
