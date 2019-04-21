//
// Created by fatih on 21.04.2019.
//

#include "Transporter.h"

Transporter::Transporter(unsigned int id, unsigned int time) : id(id), time(time), threadId(-1) {

}

void *Transporter::transporter(void *args) {
    auto *transporter = (Transporter *) args;
    transporter->threadId = pthread_self();

    pthread_exit(nullptr);
}

pthread_t Transporter::getThreadId() const {
    return threadId;
}
