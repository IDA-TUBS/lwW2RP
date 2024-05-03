# lightweight W2RP

## Features

### RTPS

* fragmentation
* uuids
* submessages (DataFrag, NackFrag, HBFrag, ...) - extentable, (de)serialization ...
* sample deadline
* reader/writerProxies
* changeForReader/Writer
* StatefulWriter/Reader only
* reader: bitmap nacks + writer: retransmissions
* QoS Policy for configuring W2RP
* (initial) xml configuration?

### W2RP

* Shaping
* Timeouts
* NACK Guard

### W2RP extensions

* multicast (prioritization, ...)
* "overlapping" sample transmissions
* dynamic sample size, "update" bitmap

### RM/HB Extensions

* ...

### Misc.

* multi port reception
* Logging/lttng tracing?
* ...

### Missing (on purpose)

* dynamic discovery
* DDS
* ...

## How to implement ...

* sockets
* threads
* ...