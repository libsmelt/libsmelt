Smelt - Machine-aware Atomic Broadcast Trees for Multi-cores
============================================================

This repository contains `libsmelt`, a library for efficient broadcast
trees for multi-core machines. 

License
-------

See LICENSE file in the repository. 

Dependencies
------------

The library requires the following tools / packages to be installed on the machine:

```
 $ sudo apt-get install cpuid hwloc libnuma-dev python3
```

It further uses the Smelt Simulator. See next step. 


Creating a machine model
------------------------

To create the model for the current machine, run the script

```
./scripts/create_model.sh
```

This will obtain the dependencies and the collects data on the machine.
You will need to run this on the machine you want to create the machine
model for.

The scripts downloads the Smelt Simulator and tools into the `./model` directory. 


Creating Overlays
-----------------

After the model has been built for a machine, the topologies for the
machine can be created using

```
./scripts/create_overlays.sh
```

This serves as a test, that the simulator is able to generate the topology
overlays for the model.


Execution
---------

There are several environment variables that control how Smelt contacts the Simulator:

- `SMLT_HOSTNAME`: The hostname of the Smelt Simulator. This should be
  localhost in most cases. Typical usage is to have an SSH tunnel to
  the Simulator or to run it on the same machine.
- `SMLT_MACHINE`: The name of the machine Smelt is running on. This
  serves as an identifier to the Simulator to find the associated
  machine model. If not set, the value given by `gethostname` will be
  used.
- `SMLT_TOPO`: The topology to be returned from the Simulator. If not
  set, `adaptivetree-shuffle-sort` will be used. Note that this
  will only be used if the call to `smlt_topology_create` uses `NULL`
  as topology name.


Building Blocks
===============

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
 * implementation to handle the events on a the queue pairs. (send and recv)


EV_RECV: receive message, read tag (possible forward), callback

# Configuration

Our Makefile supports several configurations given as environment
variables. Here is a list of them:

- `USE_FFQ`: Use FastForward rather than UMP
- `BUILDTYPE`: Supported values are `release` and `debug`. The default
  is release-mode.

