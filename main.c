#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

int roomNumbers[25]; // keep track of the room numbers assigned to each guest
int roomNumberProviders[25]; // indicates which front desk employees serviced which guests
int currentRoomNumber = 0; // keep track of the current room number
int guestThreadsCreated = 0; // keep track of the number of guest threads created
int frontDeskThreadsCreated = 0; // keep track of the number of front desk employee threads created
int bellhopThreadsCreated = 0; // keep track of the number of bellhop threads created
int guestsEntered = 0; // keep track of the number of guest threads who entered the hotel
int bagHelpers[25]; // keep track of which bellhop serviced which guests

sem_t frontDeskAvailability; // how many front desk employees are available
sem_t guestRequestedRoom; // indicates if a customer requested the front desk for a room
sem_t mutex2; // enforces mutual exclusion when processing a task on q1
sem_t mutex3; // enforces mutual exlusion when incrementing number of guest threads created
sem_t mutex4; // enforces mutual exclusion when incrementing number of front desk employee threads created
sem_t mutex5; // enforces mutual exclusion when incrementing number of guest threads who entered the hotel
sem_t mutex6; // enforces mutual exclusion when incrementing number of bellhop threads created
sem_t mutex7; // enforces mutual exclusion when processing a task on q2
sem_t finishedCreatingGuests; // indicates when all guest threads have been created
sem_t finishedCreatingFrontDesk; // indicates when all front desk employee threads have been created (for guest threads)
sem_t finishedCreatingFrontDesk2; // indicates when all front desk employee threads have been created (for bellhop threads)
sem_t finishedCreatingBellhops; // indicates when all bellhop threads have been created
sem_t finishedEnteringHotel; // indicates when all guest threads have entered the hotel
sem_t deskIsProvidingKey[25]; // indicates if a front desk employee is providing a key to a guest
sem_t guestReceivedKey[25]; // indicates if a guest received a key by the front desk
sem_t requestHelpFromBellhop; // indicates if a guest is requesting help from a bellhop to carry bags
sem_t bellhopAvailability; // how many bellhops are available
sem_t bagsTakenByBellhop[25]; // indicates when a bellhop takes the bags of a guest who requested bellhop service
sem_t guestReachedRoom[25]; // indicates to the bellhops when a guest being serviced has entered the assigned room
sem_t bagsDelivered[25]; // indicates to the guests when the bags have been delivered
sem_t tipGiven[25]; // indicates to bellhops when a guest is giving a tip
sem_t tipAccepted[25]; // indicates to the guests when a bellhop accepts a tip from the guest

// Queue implementation
void enqueue(int queue[], int* tail, int num) {
    queue[*tail + 1] = num;
    *tail = *tail + 1;
}

int dequeue(int queue[], int* head, int* tail) {
    if (*head > *tail) {
        printf("Error: Attempting to dequeue from an empty queue");
        exit(1);
    }
    
    int headNode = queue[*head];
    *head = *head + 1;
    
    return headNode;
}

// Queue for guests waiting to get checked in by the front desk 
int q1[25];
int q1Head = 0;
int q1Tail = -1;

// Queue for guests waiting for a bellhop
int q2[25];
int q2Head = 0;
int q2Tail = -1;

pthread_t thGuests[25];
pthread_t thFrontDesk[2];
pthread_t thBellHop[2];

void* guest(void* args) {
    int guestID = *(int*) args;
    printf("Guest %d created\n", guestID);
    
    // Increment guest thread created count
    sem_wait(&mutex3);
    guestThreadsCreated += 1;
    // Inform the front desk threads that the guest threads have all been created
    if (guestThreadsCreated == 25) {
        sem_post(&finishedCreatingGuests);
        sem_post(&finishedCreatingGuests);
    }
    sem_post(&mutex3);

    // Assign bag amount to each user
    int bagAmount = rand() % 5 + 1;
    
    // Wait to enter hotel until all front desk employee threads are created
    sem_wait(&finishedCreatingFrontDesk);
    // Wait to enter hotel until all bellhop threads are created
    sem_wait(&finishedCreatingBellhops);
    
    // Enter the hotel
    printf("Guest %d enters hotel with %d bags\n", guestID, bagAmount);
    sem_wait(&mutex5);
    guestsEntered += 1;
    if (guestsEntered == 25) {
        sem_post(&finishedEnteringHotel);
        sem_post(&finishedEnteringHotel);
    }
    sem_post(&mutex5);
    
    // Wait until front desk is available and request a room
    sem_wait(&frontDeskAvailability);
    sem_wait(&mutex2);
    enqueue(q1, &q1Tail, guestID);
    sem_post(&guestRequestedRoom);
    sem_post(&mutex2);
    sem_wait(&deskIsProvidingKey[guestID]);
    
    // Receive a key to hotel room
    printf("Guest %d receives room key for room %d from front desk employee %d\n", guestID, roomNumbers[guestID], roomNumberProviders[guestID]);
    sem_post(&guestReceivedKey[guestID]);
    
    if (bagAmount > 2) {
        // Wait until a bellhop is available and request help
        sem_wait(&bellhopAvailability);
        printf("Guest %d requests help with bags\n", guestID);
        sem_wait(&mutex7);
        enqueue(q2, &q2Tail, guestID);
        sem_post(&requestHelpFromBellhop);
        sem_post(&mutex7);
        
        // Wait until bellhop takes the bags off the guest
        sem_wait(&bagsTakenByBellhop[guestID]);
        
        // Go to assigned room
        printf("Guest %d enters room %d\n", guestID, roomNumbers[guestID]);
        sem_post(&guestReachedRoom[guestID]);
        
        // Wait until bellhop delivers bags to room
        sem_wait(&bagsDelivered[guestID]);
        printf("Guest %d receives bags from bellhop %d and gives tip\n", guestID, bagHelpers[guestID]);
        
        // Provide a tip to bellhop and wait until bellhop accepts the tip
        sem_post(&tipGiven[guestID]);
        sem_wait(&tipAccepted[guestID]);
        
        // Retire
        printf("Guest %d retires for the evening\n", guestID);
    }
    else {
        // Guest does not need bellhop, go to room and retire
        printf("Guest %d enters room %d\n", guestID, roomNumbers[guestID]);
        printf("Guest %d retires for the evening\n", guestID);
    }
}

void* frontDesk(void* args) {
    sem_wait(&finishedCreatingGuests);
    printf("Front desk employee %d created\n", *(int*) args);
    
    // Increment front desk thread created count
    sem_wait(&mutex4);
    frontDeskThreadsCreated += 1;
    // Inform the guest threads and bellhop threads that the front desk threads have all been created
    if (frontDeskThreadsCreated == 2) {
        for (int i = 0; i < 25; i++) {
            sem_post(&finishedCreatingFrontDesk);
        }
        sem_post(&finishedCreatingFrontDesk2);
        sem_post(&finishedCreatingFrontDesk2);
    }
    sem_post(&mutex4);
    
    // Wait until all guest threads enter the hotel
    sem_wait(&finishedEnteringHotel);
    
    int frontDeskID = *(int*) args;
    int guestNumber;
    int assignedRoom;
    
    while (1) {
        if (currentRoomNumber == 25) {
            break;
        }
        
        // Wait until a guest makes a request
        sem_wait(&guestRequestedRoom);
        
        // Assign a room to the guest
        sem_wait(&mutex2);
        guestNumber = dequeue(q1, &q1Head, &q1Tail);
        currentRoomNumber += 1;
        assignedRoom = currentRoomNumber;
        sem_post(&mutex2);
        roomNumbers[guestNumber] = assignedRoom;
        roomNumberProviders[guestNumber] = frontDeskID;
        printf("Front desk employee %d registers guest %d and assigns room %d\n", frontDeskID, guestNumber, assignedRoom);
        sem_post(&deskIsProvidingKey[guestNumber]);
        
        // Wait until the guest receives the key to the room
        sem_wait(&guestReceivedKey[guestNumber]);
        
        // Front desk employee is ready to service another guest
        sem_post(&frontDeskAvailability);
    }
}


void* bellHop(void* args) {
    // Wait until all front desk employee threads are created
    sem_wait(&finishedCreatingFrontDesk2);
    printf("Bellhop %d created\n", *(int*) args);
    
    // Increment bellhop thread created count
    sem_wait(&mutex6);
    bellhopThreadsCreated += 1;
    if (bellhopThreadsCreated == 2) {
        for (int i = 0; i < 25; i++) {
            sem_post(&finishedCreatingBellhops);
        }
    }
    sem_post(&mutex6);
    
    int bellhopID = *(int*) args;
    int guestNumber;
    
    while (1) {
        // Wait until a guest makes a request
        sem_wait(&requestHelpFromBellhop);
        
        // Get the guest number
        sem_wait(&mutex7);
        guestNumber = dequeue(q2, &q2Head, &q1Tail);
        sem_post(&mutex7);
        
        bagHelpers[guestNumber] = bellhopID;
        
        // Take the bags off the guest
        printf("Bellhop %d receives bags from guest %d\n", bellhopID, guestNumber);
        sem_post(&bagsTakenByBellhop[guestNumber]);
        
        // Wait until guest reaches room
        sem_wait(&guestReachedRoom[guestNumber]);
        
        // Deliver bags to the guest's room
        printf("Bellhop %d delivers bags to guest %d\n", bellhopID, guestNumber);
        sem_post(&bagsDelivered[guestNumber]);
        
        // Wait until guest gives tip and accept the tip
        sem_wait(&tipGiven[guestNumber]);
        sem_post(&tipAccepted[guestNumber]);
        
        // Bellhop is ready to service another guest
        sem_post(&bellhopAvailability);
    }
}


int main() {
    // For randomly generating bag number purposes
    srand(time(0));
    
    // Create all semaphores
    sem_init(&frontDeskAvailability, 0, 2);
    sem_init(&guestRequestedRoom, 0, 0);
    sem_init(&mutex2, 0, 1);
    sem_init(&mutex3, 0, 1);
    sem_init(&mutex4, 0, 1);
    sem_init(&mutex5, 0, 1);
    sem_init(&mutex6, 0, 1);
    sem_init(&mutex7, 0, 1);
    sem_init(&finishedCreatingGuests, 0, 0);
    sem_init(&finishedCreatingFrontDesk, 0, 0);
    sem_init(&finishedCreatingFrontDesk2, 0, 0);
    sem_init(&finishedCreatingBellhops, 0, 0);
    sem_init(&finishedEnteringHotel, 0, 0);
    for (int i = 0; i < 25; i++) {
        sem_init(&deskIsProvidingKey[i], 0, 0);
        sem_init(&guestReceivedKey[i], 0, 0);
        sem_init(&bagsTakenByBellhop[i], 0, 0);
        sem_init(&guestReachedRoom[i], 0, 0);
        sem_init(&bagsDelivered[i], 0, 0);
        sem_init(&tipGiven[i], 0, 0);
        sem_init(&tipAccepted[i], 0, 0);
    }
    sem_init(&requestHelpFromBellhop, 0, 0);
    sem_init(&bellhopAvailability, 0, 2);
    
    
    // Create guest, front desk employee, and bellhop threads
    for (int i = 0; i < 25; i++) {
        int* a = malloc(sizeof(int));
        *a = i;
        
        if (pthread_create(&thGuests[i], NULL, &guest, a) != 0) {
            perror("There was an error while creating the guest thread");
        }
        
        if (i < 2) {
            if (pthread_create(&thFrontDesk[i], NULL, &frontDesk, a) != 0) {
                perror("There was an error while creating the front desk employee thread");
            }
            
            if (pthread_create(&thBellHop[i], NULL, &bellHop, a) != 0) {
                perror("There was an error while creating the bellhop thread");
            }
            
        }
    }
    
    // Join all guest threads
    for (int i = 0; i < 25; i++) {
        if (pthread_join(thGuests[i], NULL) != 0) {
            perror("There was an error while joining the guest thread");
        }
        else {
            printf("Guest %d joined\n", i);
        }
    }
    
    // Destroy all semaphores
    sem_destroy(&frontDeskAvailability);
    sem_destroy(&guestRequestedRoom);
    sem_destroy(&mutex2);
    sem_destroy(&mutex3);
    sem_destroy(&mutex4);
    sem_destroy(&mutex5);
    sem_destroy(&mutex6);
    sem_destroy(&mutex7);
    sem_destroy(&finishedCreatingGuests);
    sem_destroy(&finishedCreatingFrontDesk);
    sem_destroy(&finishedCreatingFrontDesk2);
    sem_destroy(&finishedCreatingBellhops);
    sem_destroy(&finishedEnteringHotel);
    for (int i = 0; i < 25; i++) {
        sem_destroy(&deskIsProvidingKey[i]);
        sem_destroy(&guestReceivedKey[i]);
        sem_destroy(&bagsTakenByBellhop[i]);
        sem_destroy(&guestReachedRoom[i]);
        sem_destroy(&bagsDelivered[i]);
        sem_destroy(&tipGiven[i]);
        sem_destroy(&tipAccepted[i]);
    }
    sem_destroy(&requestHelpFromBellhop);
    sem_destroy(&bellhopAvailability);
    
    return 0;
}