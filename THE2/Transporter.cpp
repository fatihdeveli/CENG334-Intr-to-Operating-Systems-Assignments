//
// Created by fatih on 21.04.2019.
//

#include <iostream>
#include <zconf.h>
#include "Transporter.h"



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
    id(id), time(time), threadId(-1), carry(IRON) {

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

        std::cout << "transporter deciding " << std::endl;
        // Decide which producer to drop the carry.
        switch (transporter->carry) {
            case COPPER: // Copper always goes to smelters
                sem_wait(&producerSpacesForCopper);
                // Producers waiting for the second ore have priority over producers without ores.
                for (Smelter* s : smelters) { // Look for smelters waiting for the second ore.
                    if (s->isActive() && s->getOreType() == COPPER && s->getWaitingOreCount() == 1) {
                        // Smelter routine
                        goto outOfSwitch;
                    }
                }
                for (Smelter* s : smelters) { // Look for smelters without any ores
                    if (s->isActive() && s->getOreType() == COPPER && s->getWaitingOreCount() == 0) {
                        // Smelter routine
                        goto outOfSwitch;
                    }
                }
                break;
            case COAL: // Coal always goes to foundries
                sem_wait(&producerSpacesForCoal);
                // Producers waiting for the second ore have priority over producers without ores.
                for (Foundry* f : foundries) { // Look for foundries waiting for the second ore.
                    // TODO: consider using lock here
                    if (f->isActive() && f->getWaitingIronCount() == 1 && f->getWaitingCoalCount() == 0) {
                        // Foundry routine
                        goto outOfSwitch;
                    }
                }
                for (Foundry* f : foundries) { // Look for foundries without ores.
                    // TODO: consider using lock here
                    if (f->isActive() && f->getWaitingIronCount() == 0 && f->getWaitingCoalCount() == 0) {
                        // Foundry routine
                        goto outOfSwitch;
                    }
                }
                break;
            case IRON: // Iron goes to either a smelter or a foundry
                sem_wait(&producerSpacesForIron);
                // TODO: consider using lock here
                // Look for a smelter that already has 1 ore.
                for (Smelter* s : smelters) {
                    if (s->isActive() && s->getOreType() == IRON && s->getWaitingOreCount() == 1) {
                        // Smelter routine
                        goto outOfSwitch;
                    }
                }
                // Look for a foundry that already has 1 coal ore.
                for (Foundry* f : foundries) {
                    if (f->isActive() && f->getWaitingIronCount() == 0 && f->getWaitingCoalCount() == 1) {
                        // Smelter routine
                        goto outOfSwitch;
                    }
                }

                // Look for foundries or smelters without ores.
                // Look for a smelter without any ore.
                for (Smelter* s : smelters) {
                    if (s->isActive() && s->getOreType() == IRON && s->getWaitingOreCount() == 0) {
                        // Smelter routine
                        goto outOfSwitch;
                    }
                }
                // Look for a foundry without any ore.
                for (Foundry* f : foundries) { // Look for foundries without ores.
                    // TODO: consider using lock here
                    if (f->isActive() && f->getWaitingIronCount() == 0 && f->getWaitingCoalCount() == 0) {
                        // Foundry routine
                        goto outOfSwitch;
                    }
                }

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
    TransporterInfo transporterInfo = {id, &carry};
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
    TransporterInfo transporterInfo = {0, 0};
    WriteOutput(&minerInfo, &transporterInfo, nullptr, nullptr, TRANSPORTER_TRAVEL);

    // Sleep a value in range of Interval ± (Interval×0.01) microseconds for travel
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

    carry = miner->getOreType();

    // Notification: Transporter takes ore
    FillMinerInfo(&minerInfo, miner->getId(), miner->getOreType(), miner->getCapacity(),
            miner->getCurrentOreCount());
    FillTransporterInfo(&transporterInfo, id, &carry);
    WriteOutput(&minerInfo, &transporterInfo, nullptr, nullptr, TRANSPORTER_TAKE_ORE);

    // Sleep a value in range of Interval ± (Interval×0.01) microseconds for loading
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

    miner->signalStorageSpace();  // A new slot in the storage is available.
}

void Transporter::smelterRoutine(Smelter *smelter) {
    SmelterInfo smelterInfo;
    TransporterInfo transporterInfo;

    // Notification: transporter is traveling.
    FillSmelterInfo(&smelterInfo, smelter->getId(), intToOre(0), 0, 0, 0);
    FillTransporterInfo(&transporterInfo, 0, &carry);
    WriteOutput(nullptr, &transporterInfo, &smelterInfo, nullptr, TRANSPORTER_TRAVEL);

    // Sleep a value in range of Interval ± (Interval×0.01) microseconds for travel
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

    smelter->dropOre();

    // Notification: transporter dropped the ore
    FillSmelterInfo(&smelterInfo, smelter->getId(), smelter->getOreType(), smelter->getCapacity(),
        smelter->getWaitingOreCount(), smelter->getProducedIngotCount());
    FillTransporterInfo(&transporterInfo, id, &carry);
    WriteOutput(nullptr, &transporterInfo, &smelterInfo, nullptr, TRANSPORTER_DROP_ORE);

    // Sleep a value in range of Interval ± (Interval×0.01) microseconds for unloading
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

    smelter->signalDropOre();

    std::cout << "transporter dropped to smelter" << std::endl;
}

void Transporter::foundryRoutine(Foundry *foundry) {
    FoundryInfo foundryInfo;
    TransporterInfo transporterInfo;

    // Notification: transporter is traveling.
    FillFoundryInfo(&foundryInfo, 0, 0, 0, 0, 0);
    FillTransporterInfo(&transporterInfo, id, &carry);
    WriteOutput(nullptr, &transporterInfo, nullptr, &foundryInfo, TRANSPORTER_TRAVEL);

    // Sleep a value in range of Interval ± (Interval×0.01) microseconds for travel
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

    foundry->dropOre(carry);

    // Notification: transporter dropped the ore
    FillFoundryInfo(&foundryInfo, foundry->getId(), foundry->getCapacity(),
        foundry->getWaitingCoalCount(), foundry->getWaitingCoalCount(),
        foundry->getProducedIngotCount());
    FillTransporterInfo(&transporterInfo, id, &carry);
    WriteOutput(nullptr, &transporterInfo, nullptr, &foundryInfo, TRANSPORTER_DROP_ORE);

    // Sleep a value in range of Interval ± (Interval×0.01) microseconds for unloading
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

    foundry->signalDropOre();
    std::cout << "transporter dropped to foundry" << std::endl;
}