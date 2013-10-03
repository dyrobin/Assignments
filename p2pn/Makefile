CC= gcc
CFLAGS= -Wall -Wextra -Werror -pedantic -std=c99 $(DEBUG)
DEBUG= -ggdb

EXE= p2pn pmon
p2pn_src= p2pn.c proto.c sock_util.c util.c
pmon_src= pmon.c

ifneq ($(shell uname), Linux)
  ${error Currently only works on modern Linux systems. \
   Please check if timercmp() in p2pn.c works properly! \
   Remove OS check from Makefile if you want to proceed.}
endif

.PHONY: all
all: $(EXE)

.PHONY: clean
clean:
	$(RM) $(EXE) *.o

.SECONDEXPANSION:
$(EXE): $$(patsubst %.c,%.o,$$($$@_src))

