CFLAGS := -g -Wall $(CFLAGS)

targets = p2pn pmon

.PHONY: all
all: $(targets)

p2pn: p2pn.c proto.c sock_util.c

pmon: pmon.c

.PHONY: clean
clean:
	$(RM) $(targets) *.o
