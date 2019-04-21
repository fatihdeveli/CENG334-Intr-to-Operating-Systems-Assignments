//
// Created by fatih on 21.04.2019.
//

#include "Foundry.h"

Foundry::Foundry(unsigned int id, unsigned int interval, unsigned int capacity)
    : id(id), interval(interval), capacity(capacity), threadId(-1) {

}

void *Foundry::foundry(void *args) {
    auto *foundry = (Foundry *) args;
    foundry->threadId = pthread_self();



    pthread_exit(nullptr);
}

pthread_t Foundry::getThreadId() const {
    return threadId;
}
