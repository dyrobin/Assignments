CC= gcc
CFLAGS= -Wall $(DEBUG)
DEBUG= -ggdb

EXE= p2pn pmon
p2pn_src= p2pn.c proto.c sock_util.c
pmon_src= pmon.c

.PHONY: all
all: $(EXE)

.PHONY: clean
clean:
	$(RM) $(EXE) *.o

.SECONDEXPANSION:
$(EXE): $$(patsubst %.c,%.o,$$($$@_src))

