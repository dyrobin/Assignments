CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99 $(DEBUG_FLAG)

ifeq ($(DEBUG), 1)
    DEBUG_FLAG = -ggdb
endif

bins = p2pn pmon
p2pn_src = p2pn.c proto.c sock_util.c util.c
pmon_src = pmon.c


.PHONY: all clean
all: $(bins)

clean:
	$(RM) $(bins) *.o

.SECONDEXPANSION:
$(bins): $$(patsubst %.c,%.o,$$($$@_src))

