Executing Smelt
===============

There are several environment variables that control how Smelt contacts the Simulator:

- `SMLT_HOSTNAME`: The hostname of the Smelt Simulator. This should be
  localhost in most cases. Typical usage is to have an SSH tunnel to
  the Simulator or to run it on the same machine.
- `SMLT_MACHINE`: The name of the machine Smelt is running on. This
  serves as an identifier to the Simulator to find the associated
  machine model. If not set, the value given by `gethostname` will be
  used.
- `SMLT_TOPO`: The topology to be returned from the Simulator. If not
  set, `adaptivetree-nomm-shuffle-sort` will be used. Note that this
  will only be used if the call to `smlt_topology_create` uses `NULL`
  as topology name.


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
