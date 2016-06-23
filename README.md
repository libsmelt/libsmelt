Buildingblocks
==============

[![build status](https://gitlab.inf.ethz.ch/skaestle/smelt/badges/master/build.svg)](https://gitlab.inf.ethz.ch/skaestle/smelt/commits/master)

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


EV_RECV: receive message, read tag (possible forward), callback

# Configuration

Our Makefile supports several configurations given as environment
variables. Here is a list of them:

- `USE_FFQ`: Use FastForward rather than UMP
- `BUILDTYPE`: Supported values are `release` and `debug`. The default
  is release-mode.

# Pairwise

See NetOS machine database's README.md
