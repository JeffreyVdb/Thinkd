CC = gcc
CFLAGS = -std=gnu99 -O2 -pedantic -Wall -g
EXE = thinkd
SRCS = thinkd.c conf_utils.c acpi.c

# use macro to replace the source suffixes to .o
OBJS = $(SRCS:.c=.o)
RM = rm -f
SRCDIR = src
REAL_SRCS = $(addprefix $(SRCDIR)/, $(SRCS))
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

all: $(EXE)

$(EXE): $(OBJS)
	@echo "LINKING\t\t$(OBJS)"	
	@$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: $(SRCDIR)/%.c
	@echo "CC\t\t$< -> $@"	
	@$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: all clean killd install

killd:
	@echo "killing daemon"
	-@$(shell sudo kill $(shell sudo cat /var/run/thinkd.pid))

clean:
	$(RM) $(OBJS) $(EXE)

install: $(EXE)
	install -m 0755 $(EXE) $(BINDIR)
