//
// Created by fatih on 21.04.2019.
//

#ifndef THE2NEW_TRANSPORTER_H
#define THE2NEW_TRANSPORTER_H


#include "writeOutput.h"


class Transporter {
private:
    unsigned int id, time;
    pthread_t threadId;
public:
    pthread_t getThreadId() const;

public:
    Transporter(unsigned int id, unsigned int time);

    static void *transporter(void *args);

};


#endif //THE2NEW_TRANSPORTER_H
