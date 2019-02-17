# Ring-Orientation
Ring Orientation algorithm using mpi

<h3>Ring Orientation Algorithm:</h3>
  We have a non-oriented ring o n processes, where every process has a "next" relationship with one process in the ring and a "previous"     relationship with another one (the process makes sure it does not have the same process as both next and previous)
  This problem demands from the algorithm to manipulate the system so that for a process p, process q is p's next if and only if p is q's   previous
  
  Each process, except from a way to maintain information about its neighbors, has a state, S for "sending", R for "receiving" and I for     "internal"
  
  In each step, each process checks its state, the state of its neighbor's q and the relationship the one has with the other. The process   changes its state and/or its orientation based on a number of conditions.
  
  Processes pass on tokens to its "next" process. If two tokens colide, one of them is destroyed
  
  Finally, all remaining tokens are travelling towards the same direction, which makes the ring oriented
  
<h3>Implementation:</h3>
  The algorithm was implemented using the C programming language and the mpi interface to pass messages between each process
  To help the processes pass on messages, a repository is created to pass information to each processes about
  itself and its neighbors
  So every processes p communicates with the repository, which communicates with every neighbor q of that process and sends that             information back to p which then decides if it has to change state and/or orientation
  
<h3>Running the program:</h3>
  Since a repository is used, you need to spawn one process more than what you would ordinarily choose.
  Meaning, if you want a ring of 7 processes, you should spawn 8 processes instead
  
  To run the program you will need to install and then import (#include) mpi.h
  afterwards, you open a terminal whre your .c file is and type:
  
  ```mpicc -o my_executable_name my_c_file.c```
  
  ```mpiexec -n number_of_desired_processes+1 ./my_executable_name```
