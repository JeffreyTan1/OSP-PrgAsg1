OSP - Programming Assingment 1
S3851781
Jeffrey Zezhong Tan

##############  CODE VARIABLES  ######################################################

(1) Reader_Writer.cpp
#define NUM_THREADS
^ Set the number of readers and writers threads


(2) Sleeping_Barber.cpp
The pause time between customer arrivals is = (rand() % 1000 * 1000) / CUST_FREQ;
The output of this program is heavily dependent on the average frequency of customer arrivals.

As CUST_FREQ increases, the faster customers arrive on average (it's still random).

If the CUST_FREQ is too high for NUM_BARBERS then the output continuously spams "All seats are taken, try again later."
And barbers may be too busy to go to sleep.

If the CUST_FREQ is too low for NUM_BARBERS then the output may never reach "All seats are taken, try again later."
But we can see that barbers sleep often.

Here's some combinations (on my machine) that I've found which 
- barbers go to sleep
- waiting room is sometimes full

NUM_BARBERS = 1, CUST_FREQ = 2
NUM_BARBERS = 2, CUST_FREQ = 3
NUM_BARBERS = 3, CUST_FREQ = 4

##############  COMPILATION  ########################################################
1.
scl enable devtoolset-9 bash

2.
(1) Reader-Writer Problem
make rw
(2) Sleeping-Barber Problem
make sb

##############  EXECUTABLE  #########################################################
./simulation
