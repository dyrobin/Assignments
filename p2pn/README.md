Peer-to-Peer Node (p2pn) Application
=====

`p2pn` is an reference implementation of the custom peer-to-peer protocol for the course Application and Services in Internet (T-110.5150).


BUILDING
-----

Use `make` to compile source codes.
Use `make clean && make` for a clean build.

Some code are inlined in the header files.
If they are changed, a clean build is required.


USAGE
-----

Two executables are generated after the compilation:

```
 ./p2pn
 ./pmon
```

The `p2pn` application is the implementation of a P2P node.
The `pmon` application is a guard application to restart `p2pn` if it crashes.
The usage of each executable will be given when invoked with no arguments.

If run within GDB, you must tell GDB to not stop on SIGPIPE.

```
  $ gdb p2pn
  (gdb) handle SIGPIPE nostop pass
  (gdb) run -l ....
```

CONFIGURATION
-----

Assume that we are to deploy the following network:

```
  VM1(IP1)              VM2(IP2)                VM(IP3)
 -----------           -----------           -----------

  Node1                 Node2                 Node3
 0.0.0.0:6346          0.0.0.0:6346          0.0.0.0:6346
 key1:0x11111111       key2:0x22222222       key3:0x33333333
                       SEND QUERY "key1"     NO AUTO JOIN

```

On VM1:

```
(Node1) $ ./pmon -c "./p2pn -f kv1.txt" &> log &
(Node1) $ tail -f log
```

On VM2:

```
(Node2) $ ./pmon -c "./p2pn -f kv2.txt -b IP1:6346 -s key1" &> log &
(Node2) $ tail -f log
```

On VM3:

```
(Node3) $ ./pmon -c "./p2pn -f kv3.txt -b IP1:6346 -j" &> log &
(Node3) $ tail -f log
```

Format of `-f` input files:

```
<key:string> <value:hex>
```

One pair per line. See `kv1.txt` for an example.

 - Make sure, by testing, that the bootstrap network (VM1 and VM2 in above example) is accessible from outside Aalto network.


KNOWN ISSUES
-----

 - The implementation is based on single process, single thread I/O demultiplexing (`select()` function).
 - No IPv6 support.
 - Does not send Bye Message.
 - Does not actually handle Bye Messages. 
   The connection is disconnected because remote closes the TCP link and we have a `read()` error.
