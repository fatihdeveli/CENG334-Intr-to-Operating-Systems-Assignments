//
// Created by fatih on 21.04.2019.
//

#ifndef THE2NEW_FOUNDRY_H
#define THE2NEW_FOUNDRY_H

extern "C" {
    #include "writeOutput.h"
}

class Foundry {
private:
    unsigned int id, interval, capacity;
    pthread_t threadId;
public:
    pthread_t getThreadId() const;

public:
    Foundry(unsigned int id, unsigned int interval, unsigned int capacity);
    static void *foundry(void *args);

};


#endif //THE2NEW_FOUNDRY_H
