Each region (north, south, east, west) has a separate process. 
These processes update their weather data to a shared memory segment every few seconds.
A control process manages these region processes — restarting any that crash or send failure signals.

Shared memory enables IPC while semaphores ensure safe concurrent access.
A round-robin load balancing strategy determines which region to update/display per cycle.

run these files

gcc init.c -o init
gcc control_process.c -o control_process 
gcc north_process.c -o north_process 
gcc south_process.c -o south_process 
gcc east_process.c -o east_process 
gcc west_process.c -o west_process

run the initializer to create the semaphore and shared memory using
./init

Then run the controller code using [ ./control_process ]
It Launch north_process, south_process, east_process, and west_process

It Monitors all region PIDs

It restarts any process that fails.


