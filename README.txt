This project deals with concurrency between threads using semaphores.
This project is inspired from Dijkstra's barbershop problem.

This project simulates a hotel with three main thread entities - guests, front desk employees, and bellhops.
The hotel to be simulated has two employees at the front desk to register guests and two bellhops to handle guests’ bags.
A guest will first visit the front desk to get a room number.
The front desk employee will find an available room and assign it to the guest.
If the guest has less than 3 bags, the guest proceeds directly to the room.
Otherwise, the guest visits the bellhop to drop off the bags.
The guest will later meet the bellhop in the room to get the bags, at which time a tip is given.

Threads:

Guest:
25 guests visit the hotel (1 thread per guest created at start of simulation).
Each guest has a random number of bags (0-5).
A guest must check in to the hotel at the front desk.
Upon check in, a guest gets a room number from the front desk employee.
A guest with more than 2 bags requires a bellhop.
The guest enters the assigned room.
Receives bags from bellhop and gives tip (if more than 2 bags).
Retires for the evening.

Front Desk:
Two employees at the front desk (1 thread each).
Checks in a guest, finds available room, and gives room number to guest.

Bellhop:
Two bellhops (1 thread each).
Gets bags from guest.
The same bellhop that took the bags delivers the bags to the guest after the guest is in the room.
Accepts tip from guest.

All the code is in the main.c file. To run, enter the following commands:
gcc -pthread -std=c99 main.c
./a.out main.c