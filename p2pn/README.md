Peer-to-Peer Node (p2pn) Application

The p2pn is an reference implementation of the home-brew peer-to-peer protocol
for the course Application and services in Internet (T-110.5150).

COMPILATION
==================================
Use "make" to compile source codes.
Use "make clean && make" for a clean build.

Some code are inlined in the header files.
If they are changed, a clean build is required.


USAGE
==================================
Two executables are provided after the compilation:
    ./p2pn
    ./pmon

The p2pn application is the implementation of a p2p node.
The pmon application is a guard application to restart the p2pn if it crashes.
The usage of each executable will be given when input the above commands.

If run within GDB, you must tell GDB to not stop on SIGPIPE.

  $ gdb p2pn
  (gdb) handle SIGPIPE nostop pass
  (gdb) run -l ....

SCENARIO
==================================
Let us assume that we want to deploy three nodes (N1, N2, N3) with address
IP1:PORT1, IP2:PORT2, IP3:PORT3, respectively. N1 stores key/value pairs in
the "kv.txt" file. N2 connects to N1, and N3 connects to N2. N3 wants to
search the value of the key "testkey". We can deploy the network by input
commands as follow:

N1:
$ ./p2pn -l IP1:PORT1 -f kv.txt

N2:
$ ./p2pn -l IP2:PORT2 -b IP1:PORT1

N3:
$ ./p2pn -l IP3:PORT3 -b IP2:PORT2 -s testkey

N4:
$ ./p2pn -l IP4:PORT4 -b IP1:PORT1 -f kv1.txt


KNOWN ISSUES
==================================
1. The implementation is based on single-process single-thread I/O
demultiplexing (the "select()" function in POSIX), but the function
"connect_pto()" will temporarily block the whole process for establishing
a new connection.

2. No IPv6 support.
3. Resource value must not be zero.
4. Does not actually handle Bye Messages.
