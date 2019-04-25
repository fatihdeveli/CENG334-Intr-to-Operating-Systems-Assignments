//
// Created by fatih on 21.04.2019.
//

#include <iostream>
#include <zconf.h>
#include "Transporter.h"
#include "Miner.h"
#include "Smelter.h"
#include "Foundry.h"


// Transporter needs to check if miners are active and if they have ores in their storage.
extern std::vector<Miner*> miners;
extern std::vector<Smelter *> smelters;
extern std::vector<Foundry *> foundries;

// Transporter needs to convert its integer carry type (initially -1) to OreType
extern OreType intToOre(unsigned int oreType);

extern sem_t producedOres;
extern sem_t producerSpacesForCopper;
extern sem_t producerSpacesForCoal;
extern sem_t producerSpacesForIron;

Transporter::Transporter(unsigned int id, unsigned int time) :
    id(id), time(time), threadId(-1), carry(-1) {

    pthread_create(&threadId, nullptr, transporter, this);

}

void *Transporter::transporter(void *args) {
    auto *transporter = (Transporter *) args;
    transporter->threadId = pthread_self();

    int minerIndex = 0; // Need to iterate over miners

    std::cout << "transporter knows miners: " << miners.size() << " elements." << std::endl;

    transporter->writeTransporterOutput(TRANSPORTER_CREATED);


    while (activeMinerExists(miners) || minerWithOresExist(miners)) {
        // Carried ore type is initially -1.

        // Find the next miner with available ores in the storage. Iterate no more than maximum miners.size()
        // times since another thread may have taken the last ore in a storage since last check.
        sem_wait(&producedOres);
        for (int i = 0; i < miners.size(); i++) {
            pthread_mutex_lock(&miners[minerIndex]->oreCountMutex);
            if (miners[minerIndex]->getCurrentOreCount() - miners[minerIndex]->getReservedOreCount() > 0) {
                miners[minerIndex]->reserveOre();
                pthread_mutex_unlock(&miners[minerIndex]->oreCountMutex);

                transporter->minerRoutine(miners[minerIndex]);

                minerIndex = (minerIndex+1) % miners.size();
                break;
            }
            minerIndex = (minerIndex+1) % miners.size();
        }


        switch (transporter->carry) {
            case COPPER: // Goes to smelters
                sem_wait(&producerSpacesForCopper);
                // Producers waiting for the second ore have priority over producers without ores.
                for (Smelter* s : smelters) { // Look for smelters waiting for the second ore.
                    if (s->getOreType() == COPPER && s->getWaitingOreCount() == 1) {
                        // Smelter routine
                        goto outOfSwitch;
                    }
                }
                for (Smelter* s : smelters) { // Look for smelters without any ores
                    if (s->getOreType() == COPPER && s->getWaitingOreCount() == 0) {
                        // Smelter routine
                        goto outOfSwitch;
                    }
                }
                break;
            case COAL: // Goes to foundries
                sem_wait(&producerSpacesForCoal);
                // Producers waiting for the second ore have priority over producers without ores.
                for (Foundry* f : foundries) { // Look for foundries waiting for the second ore.
                    // TODO: consider using lock here
                    if (f->getWaitingIronCount() == 1 && f->getWaitingCoalCount() == 0) {
                        // Foundry routine
                        goto outOfSwitch;
                    }
                }
                for (Foundry* f : foundries) { // Look for foundries without ores.
                    // TODO: consider using lock here
                    if (f->getWaitingIronCount() == 0 && f->getWaitingCoalCount() == 0) {
                        // Foundry routine
                        goto outOfSwitch;
                    }
                }
                break;
            case IRON: // Goes to either a smelter or a foundry
                sem_wait(&producerSpacesForIron);
                // TODO: consider using lock here
                // Look for foundries or smelters that already have 1 ore.

                // Look for foundries or smelters without ores.

                break;

        }
        outOfSwitch:
        ;


        




    }

    /* If miners have stopped and there are no more ores, check if other threads are waiting.
     * Wake up other transporter threads if they are waiting. */
    int semValue;
    sem_getvalue(&producedOres, &semValue);

    std::cout << "waking up " << -1*semValue << " threads." << std::endl;
    for (; semValue < 0; semValue++) {
        sem_post(&producedOres);
    }

    pthread_exit(nullptr);
}

pthread_t Transporter::getThreadId() const {
    return threadId;
}

void Transporter::writeTransporterOutput(Action action) {
    OreType oreType = intToOre(carry);
    TransporterInfo transporterInfo = {id, &oreType};
    WriteOutput(nullptr, &transporterInfo, nullptr, nullptr, action);
}

bool Transporter::activeMinerExists(std::vector<Miner *> &minerList) {
    for (Miner *m : minerList) {
        if (m->isActive()) {
            return true;
        }
    }
    return false;
}

bool Transporter::minerWithOresExist(std::vector<Miner *> &minerList) {
    for (Miner *m : minerList) {
        if (m->getCurrentOreCount() > 0) {
            return true;
        }
    }
    return false;
}

void Transporter::minerRoutine(Miner *miner) {
    // Notification: Transporter travel
    MinerInfo minerInfo = {id, intToOre(0), 0, 0};
    TransporterInfo transporterInfo;
    WriteOutput(&minerInfo, &transporterInfo, nullptr, nullptr, TRANSPORTER_TRAVEL);

    // Sleep a value in range of Interval ± (Interval×0.01) microseconds for travel
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

    carry = miner->getOreType();

    // Notification: Transporter takes ore
    FillMinerInfo(&minerInfo, miner->getId(), miner->getOreType(), miner->getCapacity(),
            miner->getCurrentOreCount());
    OreType carryType = intToOre(carry);
    FillTransporterInfo(&transporterInfo, id, &carryType);
    WriteOutput(&minerInfo, &transporterInfo, nullptr, nullptr, TRANSPORTER_TAKE_ORE);

    // Sleep a value in range of Interval ± (Interval×0.01) microseconds for loading
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

    miner->signalStorageSpace();  // A new slot in the storage is available.
}
