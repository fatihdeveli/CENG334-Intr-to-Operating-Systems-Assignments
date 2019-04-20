#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err34-c"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <zconf.h>
#include "writeOutput.h"


typedef struct MinerArgs { // Arguments for miner thread function
    unsigned int id, interval, capacity, totalOre;
    OreType oreType;
} MinerArgs;

typedef struct TransporterArgs { // Arguments for transporter thread function
    unsigned int id, time;
} TransporterArgs;

typedef struct SmelterArgs { // Arguments for smelter thread function
    unsigned int id, interval, capacity;
    OreType type;
} SmelterArgs;

typedef struct FoundryArgs { // Arguments for foundry thread function
    unsigned int id, interval, capacity;
} FoundryArgs;


OreType intToOre(unsigned int i);
void *miner(void *args);        // Miner thread function
void *transporter(void *args);  // Transporter thread function
void *smelter(void *args);      // Smelter thread function
void *foundry(void *args);      // Foundry thread function

sem_t *minerSlots; // Array of semaphores for the slots in the storage of miners.
unsigned int *currentOreCounts; // Ore counts in the storage of miners.
pthread_mutex_t *minerOreCountMutexes; // Mutexes for the ore counts of miners.
sem_t *producedOres; // Array of semaphores for the ores produced by miners.


// TODO: fix the makefile
int main() {

    InitWriteOutput();

    //// Create miner threads
    int Nm; // Number of miners
    scanf("%d", &Nm);

    pthread_t minerThreadIds[Nm];

    // Available slots in the storage of miners.
    minerSlots = (sem_t *) malloc(sizeof(sem_t) * Nm);
    // TODO: Destroy semaphore, free allocated space.

    currentOreCounts = (unsigned int *) malloc(sizeof(unsigned int) * Nm); // TODO: free
    minerOreCountMutexes = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t) * Nm); // TODO: free
    producedOres = (sem_t *) malloc(sizeof(sem_t) * Nm); // TODO: free


    for (int i = 0; i < Nm; i++) {
        // Read inputs and prepare arguments struct for thread function
        MinerArgs *args = (MinerArgs *) malloc(sizeof(MinerArgs));
        args->id = i+1; // ID's start from 1
        scanf("%d", &args->interval);
        scanf("%d", &args->capacity);
        int type;
        scanf("%d", &type);
        args->oreType = intToOre(type);
        scanf("%d", &args->totalOre);

        // Initialize the semaphores
        sem_init(&minerSlots[i], 0, args->capacity); // Start with empty storage (all slots are available)
        sem_init(&producedOres[i], 0, 0); // Start with 0 produced ore.


        pthread_create(&minerThreadIds[i], NULL, miner, args);
    }

    /// Create transporter threads
    int Nt; // Number of transporters
    scanf("%d", &Nt);
    pthread_t transporterThreadIds[Nt];
    for (int i = 0; i < Nt; i++) {
        TransporterArgs *args = (TransporterArgs *) malloc(sizeof(TransporterArgs));
        args->id = i+1;
        scanf("%d", &args->time);

        pthread_create(&transporterThreadIds[i], NULL, transporter, args);
    }

    /// Create smelter threads
    int Ns; // Number of smelters
    scanf("%d", &Ns);
    pthread_t smelterThreadIds[Ns];
    for (int i = 0; i < Ns; i++) {
        SmelterArgs *args = (SmelterArgs *) malloc(sizeof(SmelterArgs));
        args->id = i+1;
        scanf("%d", &args->interval);
        scanf("%d", &args->capacity);
        int oreType;
        scanf("%d", &oreType);
        args->type = intToOre(oreType);

        pthread_create(&smelterThreadIds[i], NULL, smelter, args);
    }

    /// Create foundry threads
    int Nf; // Number of foundries
    scanf("%d", &Nf);
    pthread_t foundryThreadIds[Nf];
    for (int i = 0; i < Nf; i++) {
        FoundryArgs *args = (FoundryArgs *) malloc(sizeof(FoundryArgs));
        args->id = i+1;
        scanf("%d", &args->interval);
        scanf("%d", &args->capacity);

        pthread_create(&foundryThreadIds[i], NULL, foundry, args);
    }



    // wait for threads to exit
    for (int i = 0; i < Nm; i++) pthread_join(minerThreadIds[i], NULL);
    for (int i = 0; i < Nt; i++) pthread_join(transporterThreadIds[i], NULL);
    for (int i = 0; i < Ns; i++) pthread_join(smelterThreadIds[i], NULL);
    for (int i = 0; i < Nf; i++) pthread_join(foundryThreadIds[i], NULL);

    return 0;
}


void *miner(void *arg) {
    MinerArgs *args = (MinerArgs *) arg;
    printf("Created miner, id: %d, capacity: %d, interval: %d, totalOre: %d, oreType: %d\n",
            args->id, args->capacity, args->interval, args->totalOre, args->oreType);

    unsigned int id = args->id;
    unsigned int capacity = args->capacity;
    unsigned int interval = args->interval;
    unsigned int totalOre = args->totalOre;
    OreType oreType = args->oreType;
    free(args);
    args = NULL; arg = NULL;

    // Start with 0 ores in the storage.
    currentOreCounts[id] = 0;

    // Notification: Miner created
    MinerInfo miner;
    FillMinerInfo(&miner, id, oreType, capacity, currentOreCounts[id]);
    WriteOutput(&miner, NULL, NULL, NULL, MINER_CREATED);

    while (totalOre) { // While there are remaining ore in the mine
        // Wait until a storage space is cleared by a transporter and reserve a storage
        // space for the next ore.
        sem_wait(&minerSlots[id]);

        WriteOutput(&miner, NULL, NULL, NULL, MINER_STARTED);

        // Sleep a value in range of Interval +- (Interval*0.01) microseconds for production.
        usleep(interval - (interval*0.01) + (rand()%(int)(interval*0.02)));

        // Produced an ore, increment currentOreCount.
        pthread_mutex_lock(&minerOreCountMutexes[id]);
        currentOreCounts[id] += 1;
        pthread_mutex_unlock(&minerOreCountMutexes[id]);

        // Produced an ore, inform available transporters
        sem_post(&producedOres[id]);

        // Notification: Miner finished
        FillMinerInfo(&miner, id, oreType, capacity, currentOreCounts[id]);
        WriteOutput(&miner, NULL, NULL, NULL, MINER_FINISHED);

        // Sleep a value in range of Interval +- (Interval*0.01) microseconds for the next round.
        usleep(interval - (interval*0.01) + (rand()%(int)(interval*0.02)));

        totalOre--;
    }

    // Quit upon producing totalOre amount of ores.

    // MinerStopped() TODO: ?


    FillMinerInfo(&miner, id, oreType, capacity, currentOreCounts[id]);
    WriteOutput(&miner, NULL, NULL, NULL, MINER_STOPPED);
    pthread_exit(0);
}

void *transporter(void *arg) {
    TransporterArgs *args = (TransporterArgs *) arg;
    printf("Created transporter, id: %d, time: %d\n", args->id, args->time);

    TransporterInfo transporter;
    FillTransporterInfo(&transporter, args->id, NULL);
    WriteOutput(NULL, &transporter, NULL, NULL, TRANSPORTER_CREATED);

    // Start with 0 ores in the storage.

    // Visit miners in id order

    // Quit if there are no active miners, or miners with ores in their storage

    // Carry coppers to smelters, irons to smelter or foundry. No preference.

    // Priority for producers waiting for the second ore over producers with no ore.

    // Carry coals to foundry


    free(arg);
    WriteOutput(NULL, &transporter, NULL, NULL, TRANSPORTER_STOPPED);
    pthread_exit(0);
}

void *smelter(void *arg) {
    SmelterArgs *args = (SmelterArgs *) arg;
    printf("Created smelter, id: %d, capacity: %d, type: %d, interval: %d\n",
            args->id, args->capacity, args->type, args->interval);

    SmelterInfo smelter;
    FillSmelterInfo(&smelter, args->id, args->type, args->capacity, 0, 0);
    WriteOutput(NULL, NULL, &smelter, NULL, SMELTER_CREATED);


    // Start with 0 ores in the storage.

    // Require 2 ores to produce 1 ingot. Wait for ores to be deposited

    // Quit if cannot produce ingots for a certain duration


    free(arg);
    WriteOutput(NULL, NULL, &smelter, NULL, SMELTER_STOPPED);
    pthread_exit(0);
}

void *foundry(void *arg) {
    FoundryArgs *args = (FoundryArgs *) arg;
    printf("Created foundry, id: %d, interval: %d, capacity: %d\n",
            args->id, args->interval, args->capacity);

    FoundryInfo foundry;
    FillFoundryInfo(&foundry, args->id, args->capacity, 0, 0, 0);
    WriteOutput(NULL, NULL, NULL, &foundry, FOUNDRY_CREATED);

    // Start with 0 ores in the storage.

    // Require 1 iron and 1 coal to produce 1 steel ingot. Wait for ores to be
    // deposited

    // Quit if cannot produce ingots for a certain duration

    free(arg);
    WriteOutput(NULL, NULL, NULL, &foundry, FOUNDRY_STOPPED);
    pthread_exit(0);
}

OreType intToOre(unsigned int i) {
    switch (i) {
        case 0:
            return IRON;
        case 1:
            return COPPER;
        case 2:
            return COAL;
        default: // Normally code should never reach here
            printf("Error converting int to OreType.\n");
            return IRON;
    }
}
#pragma clang diagnostic pop