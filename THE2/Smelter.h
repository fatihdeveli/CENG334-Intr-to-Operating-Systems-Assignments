//
// Created by fatih on 21.04.2019.
//

#ifndef THE2NEW_SMELTER_H
#define THE2NEW_SMELTER_H


#include "writeOutput.h"


class Smelter {
private:
    unsigned int id, interval, capacity, producedIngotCount;
    OreType oreType;
    pthread_t threadId;
public:
    pthread_t getThreadId() const;

public:
    Smelter(unsigned int id, unsigned int interval, unsigned int capacity, OreType oreType);
    static void *smelter(void* args);
};


#endif //THE2NEW_SMELTER_H
