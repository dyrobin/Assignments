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

`pmon` was not used during 2013 Autumn term and instances of `p2pn` were run withing GDB to facilitate debugging.


CONFIGURATION
-----

Assume that we are to deploy the following network:

```
  VM1(IP1)              VM2(IP2)
 -----------           -----------

  Node1                 Node2
 0.0.0.0:10001    ===  0.0.0.0:100002
 key1:0x12345678       key3:0x98765432
                       SEND QUERY "testkey"
  |

  Node2
 127.0.0.1:10003
 key2:0x43219876
 NO AUTO JOIN
```

On VM1:

```
(Node1) $ ./p2pn -l 0.0.0.0:10001 -f k1.txt
(Node2) $ ./p2pn -l 127.0.0.1:10003 -f k2.txt -b 127.0.0.1:10001 -j
```

On VM2:

```
(Node3) $ ./p2pn -l 0.0.0.0:10002 -f k3.txt -b IP1:10001 -s testkey
```

Format of `-f` input files:

```
<key:string> <value:hex>
```

One pair per line. See `kv.txt` for an example.

 - Make sure, by testing, that the bootstrap network (VM1 and VM2 in above example) is accessible from outside Aalto network.


KNOWN ISSUES
-----

 - The implementation is based on single process, single thread I/O demultiplexing (`select()` function).
   However, `connect\_pto()` will temporarily block the whole process when establishing a new connection.
 - No IPv6 support.
 - Resource value must not be zero.
 - Does not send Bye Message.
 - Does not actually handle Bye Messages.
   The connection is disconnected because remote closed the TCP link and we had a `read()` error.

