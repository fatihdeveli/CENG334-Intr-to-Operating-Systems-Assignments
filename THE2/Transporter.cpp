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
    id(id), time(time), threadId(-1),
    carry(IRON) // Initial ore type is not used.
{
    pthread_create(&threadId, nullptr, transporter, this);
}

void *Transporter::transporter(void *args) {

    sleep(1);
    auto *transporter = (Transporter *) args;
    transporter->threadId = pthread_self();

    int minerIndex = 0; // Need to iterate over miners

    transporter->writeTransporterOutput(TRANSPORTER_CREATED);

    while (activeMinerExists(miners) || minerWithOresExist(miners)) {

        sem_wait(&producedOres); // Wait for a miner to produce an ore.

        // Find the next miner with available ores in the storage. Iterate no more than maximum miners.size()
        // times since another thread may have taken the last ore in a storage since last check.

        bool pickedUpOre = false;
        for (int i = 0; i < miners.size(); i++) {
            pthread_mutex_lock(&miners[minerIndex]->oreCountMutex);
            if (miners[minerIndex]->getCurrentOreCount() - miners[minerIndex]->getReservedOreCount() > 0) {
                miners[minerIndex]->reserveOre();
                pthread_mutex_unlock(&miners[minerIndex]->oreCountMutex);

                transporter->minerRoutine(miners[minerIndex]);

                minerIndex = (minerIndex+1) % miners.size();
                pickedUpOre = true;
                break;
            }
            pthread_mutex_unlock(&miners[minerIndex]->oreCountMutex);
            minerIndex = (minerIndex+1) % miners.size();
        }
        if (!pickedUpOre) continue; // Another thread may have taken the only ore.

        // Decide which producer to drop the carry.
        switch (transporter->carry) {
            case COPPER: // Copper always goes to smelters
                sem_wait(&producerSpacesForCopper);
                // Producers waiting for the second ore have priority over producers without ores.
                for (Smelter* s : smelters) { // Look for smelters waiting for the second ore.
                    if (s->isActive() && s->getOreType() == COPPER && s->getWaitingOreCount() == 1) {
                        transporter->smelterRoutine(s);
                        goto outOfSwitch;
                    }
                }
                for (Smelter* s : smelters) { // Look for smelters without any ores
                    if (s->isActive() && s->getOreType() == COPPER && s->getWaitingOreCount() == 0) {
                        transporter->smelterRoutine(s);
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
                        transporter->foundryRoutine(f);
                        goto outOfSwitch;
                    }
                }
                for (Foundry* f : foundries) { // Look for foundries without ores.
                    // TODO: consider using lock here
                    if (f->isActive() && f->getWaitingIronCount() == 0 && f->getWaitingCoalCount() == 0) {
                        transporter->foundryRoutine(f);
                        goto outOfSwitch;
                    }
                }
                break;

            case IRON: // Iron goes to either a smelter or a foundry
                sem_wait(&producerSpacesForIron);
                // TODO: consider using lock here
                // Look for a foundry that has coal, waiting for the iron ore.
                for (Foundry* f : foundries) {
                    if (f->isActive() && f->getWaitingIronCount() == 0 && f->getWaitingCoalCount() >= 1) {
                        transporter->foundryRoutine(f);
                        goto outOfSwitch;
                    }
                }
                // Look for a smelter that is waiting for the second ore.
                for (Smelter* s : smelters) {
                    if (s->isActive() && s->getOreType() == IRON && s->getWaitingOreCount() == 1) {
                        transporter->smelterRoutine(s);
                        goto outOfSwitch;
                    }
                }

                // Look for foundries or smelters without ores.
                // Look for a foundry without any ore.
                for (Foundry* f : foundries) { // Look for foundries without ores.
                    // TODO: consider using lock here
                    if (f->isActive() && f->getWaitingIronCount() == 0 && f->getWaitingCoalCount() == 0) {
                        transporter->foundryRoutine(f);
                        goto outOfSwitch;
                    }
                }
                // Look for a smelter without any ore.
                for (Smelter* s : smelters) {
                    if (s->isActive() && s->getOreType() == IRON && s->getWaitingOreCount() == 0) {
                        transporter->smelterRoutine(s);
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

    for (; semValue <= 0; semValue++) {
        sem_post(&producedOres);
    }

    /* If producers have stopped, check if other transporter threads are waiting for
     * producers. Wake up other transporter threads if they are waiting. */
    sem_getvalue(&producerSpacesForIron, &semValue);
    for (; semValue <= 0; semValue++) {
        sem_post(&producerSpacesForIron);
    }
    sem_getvalue(&producerSpacesForCopper, &semValue);
    for (; semValue <= 0; semValue++) {
        sem_post(&producerSpacesForCopper);
    }
    sem_getvalue(&producerSpacesForCoal, &semValue);
    for (; semValue <= 0; semValue++) {
        sem_post(&producerSpacesForCoal);
    }

    // Notification: transporter stopped.
    transporter->writeTransporterOutput(TRANSPORTER_STOPPED);

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
    TransporterInfo transporterInfo = {id, &carry};
    WriteOutput(&minerInfo, &transporterInfo, nullptr, nullptr, TRANSPORTER_TRAVEL);

    // Sleep a value in range of Interval ± (Interval×0.01) microseconds for travel
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

    carry = miner->getOreType();
    miner->pickUpOre();

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
    FillTransporterInfo(&transporterInfo, id, &carry);
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
        foundry->getWaitingCoalCount(), foundry->getWaitingIronCount(),
        foundry->getProducedIngotCount());
    FillTransporterInfo(&transporterInfo, id, &carry);
    WriteOutput(nullptr, &transporterInfo, nullptr, &foundryInfo, TRANSPORTER_DROP_ORE);

    // Sleep a value in range of Interval ± (Interval×0.01) microseconds for unloading
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

    foundry->signalDropOre();
}

bool Transporter::activeProducerExist(const std::vector<Smelter *> &smtList, const std::vector<Foundry *> fndList) {
    for (Smelter *s : smtList) {
        if (s->isActive())
            return true;
    }
    for (Foundry *f : fndList) {
        if (f->isActive())
            return true;
    }
    return false;
}
