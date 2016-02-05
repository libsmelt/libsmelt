Buildingblocks
==============

+--------------+       +----------+
|              |       |          |
|      +----+  |       | +----+   |
|      | EP |============| EP |   |
|      +----+  |  QP   | +----+   |
+--------------+       +----------+
   Node 0                Node 1


Endpoint (EP)
-------------
 * Communication abstraction "address"
 * Handles the actual send and receive of messages

Queuepair (QP)
--------------
 * represents a connection between two nodes
 * two endpoints form a queue pair

Instance
--------
 * a particular Smelt configuration: how the nodes communicate 
 * queue pair specification


Eventsets / Waitsets
--------------------
 * implementationto handle the events on a the queue pairs. (send and recv)


# Configuration

Our Makefile supports several configurations given as environment
variables. Here is a list of them:

- `USE_FFQ`: Use FastForward rather than UMP
- `BUILDTYPE`: Supported values are `release` and `debug`. The default
  is release-mode.

# Pairwise

This is how to generate pairwise measurements on Linux.

 - Compile `bench/pairwise_raw.cpp`
 - Execute the binary to the target machine: `ssh -t $M ./pairwise |
   tee $DIR/$M/pairwise_raw`, where `$M` is the target machine and
   `$DIR` the directory of the NetOS machine database. Make sure
   required libraries (such as libnuma) are available on that machine.
 - Process raw output file with `sk_m_parser.py`
 - libsync's `./evaluate-pairwise.py <machine>` to read raw output and
   store in machine DB. 
   - This generates `<machine>/pairwise_send` and `<machine>/pairwise_receive`
 - `./generate-pairwise.py` in Simulator to render images
