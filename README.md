# processesCoordinator

[Operating Systems](https://www.di.uoa.gr/en/studies/undergraduate/112) @ [University of Athens](https://www.di.uoa.gr/en) Winter 2019

## 0 Usage
To compile use:

`make`

To run use:

`./coordinator <NumOfPeers> <NumOfEntries> <NumOfReps> <PercentOfReads>`

  * `<NumOfPeers>` : the number of different peers accessing the shared memory space
  * `<NumOfEntries>` : the number of entries in memory
  * `<NumOfReps>` : the number of times each peer will try to access an entry
  * `<PercentOfReads>` : the percentage of Readers vs Writers (give an integer for ex. '70' is you want 70% to be readers)


## 1 Description
The following program creates and coordinates processes in order to explore the classic readers-writers problem through means of inter process communication.

The main program is a coordinator
The peers (processes) are activated for a specific number of cycles. In every cycle a peers attempts to access one of the entries in the main memory. A peer can either be a reader or a writer. Multiple readers can simultaniusly have access to the same entry. If a writer has access to the entry no other peers can access it. Entries are occupied for a random exponentially distributed amount of time. The coordinator computes the mean waiting time for a process to access an entry as well as the number of writting/reading attempts.

After a peer has completed all its cycles of a specific peer the accumulated statistics are printed. Once all the peers have finished their work the coordinator presents the overall statistics and de-allocates all allocated memory.
