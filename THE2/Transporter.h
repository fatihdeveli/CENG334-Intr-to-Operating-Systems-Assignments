//
// Created by fatih on 21.04.2019.
//

#ifndef THE2NEW_TRANSPORTER_H
#define THE2NEW_TRANSPORTER_H

#include <vector>
#include "Miner.h"
#include "Smelter.h"
#include "Foundry.h"

extern "C" {
#include "writeOutput.h"
}


class Transporter {
private:
    unsigned int id, time;
    OreType carry;
    pthread_t threadId;
    void writeTransporterOutput(Action action);
    void minerRoutine(Miner * miner);
    void smelterRoutine(Smelter *smelter);
    void foundryRoutine(Foundry *foundry);

public:
    Transporter(unsigned int id, unsigned int time);
    pthread_t getThreadId() const;

    // Check if there is at least one miner
    static bool activeMinerExists(std::vector<Miner*> &miners);

    // Check if there are ores in the storage of the miners
    static bool minerWithOresExist(std::vector<Miner *> &miners);
    

    static void *transporter(void *args);

};


#endif //THE2NEW_TRANSPORTER_H
