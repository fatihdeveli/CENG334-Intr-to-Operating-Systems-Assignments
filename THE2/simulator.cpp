#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include "writeOutput.h"

using namespace std;


typedef struct MinerArgs
{
    unsigned int id;
    unsigned int interval;
    unsigned int capacity;
    OreType oreType;
    unsigned int totalOre;
} MinerArgs;

OreType intToOre(unsigned int i);
void *miner(void *args);
void transporter(unsigned int id, unsigned int time);
void smelter(unsigned int id, unsigned int interval, unsigned int capacity, OreType type);

void *foo(void *p){
    return 0;
}



// TODO: fix the makefile
int main() {
    int Nm; // Number of miners
    int Nt; // Number of transporters
    int Ns; // Number of smelters
    int Nf; // Number of foundries

    cin >> Nm;
    cout << "Nm" << endl;
    for (int i = 1; i <= Nm; i++) {
        MinerArgs *args = (MinerArgs *) malloc(sizeof(MinerArgs));
        unsigned int interval, capacity, oreType, totalOre;
        cin >> args->interval;
        cin >> args->capacity;
        int type;
        cin >> type;
        args->oreType = intToOre(type);
        cin >> args->totalOre;

        // TODO: start miner thread
        pthread_t p;
        pthread_create(&p, NULL, foo, NULL);

        pthread_t m;
        pthread_create(&m, NULL, miner, &args);
        
    }

    cin >> Nt;
    cout << "Nt" << endl;
    for (int i = 1; i <= Nt; i++) {
        unsigned int time; // Travel/load/unload time in microseconds
        cin >> time;

        // TODO: start transporter thread
        transporter(i, time);
    }

    cin >> Ns;
    cout << "Ns" << endl;
    for (int i = 1; i <= Nt; i++) {
        unsigned int interval, capacity, oreType;
        cin >> interval;
        cin >> capacity;
        cin >> oreType;
        // TODO: start smelter thread
        smelter(i, interval, capacity, intToOre(oreType));
    }

    cin >> Nf;
    cout << "Nf" << endl;
    for (int i = 1; i <= Nf; i++) {
        unsigned int interval, capacity;
        cin >> interval;
        cin >> capacity;

        // TODO: start foundry thread
    }
    
    return 0;
}

void *miner(void *arg) {
    cout << "miner thread started" << endl;
    MinerArgs *argp = (MinerArgs *) arg;
    MinerInfo miner;
    FillMinerInfo(&miner, argp->id, argp->oreType, argp->capacity, 0);
    WriteOutput(&miner, NULL, NULL, NULL, MINER_CREATED);
    return NULL;
}

void transporter(unsigned int id, unsigned int time) {
    TransporterInfo transporter;
    FillTransporterInfo(&transporter, id, NULL);
    WriteOutput(NULL, &transporter, NULL, NULL, TRANSPORTER_CREATED);
    return;
}

void smelter(unsigned int id, unsigned int interval, unsigned int capacity, OreType type) {
    SmelterInfo smelter;
    FillSmelterInfo(&smelter, id, type, capacity, 0, 0);
    WriteOutput(NULL, NULL, &smelter, NULL, SMELTER_CREATED);
    return;
}

OreType intToOre(unsigned int i) {
    switch (i) {
        case 0:
            return IRON;
        case 1:
            return COPPER;
        case 2:
            return COAL;
    }
}