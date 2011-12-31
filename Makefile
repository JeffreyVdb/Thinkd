CC = gcc
CFLAGS = -std=gnu99 -O2 -pedantic -Wall -g
EXE = thinkd
SRCS = thinkd.c conf_utils.c acpi.c logger.c

# use macro to replace the source suffixes to .o
OBJS = $(SRCS:.c=.o)
RM = rm -f
SRCDIR = src
INITDIR = init.d
REAL_SRCS = $(addprefix $(SRCDIR)/, $(SRCS))
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

all: $(EXE)

$(EXE): .gitignore $(OBJS)
	@echo "LINK $(EXE) <- $(OBJS)"	
	$(shell echo $@ >>.gitignore)
	@$(CC) $(CFLAGS) -o $@ $(OBJS)
	@echo "STRIP $(EXE)"
	@strip --strip-all $(EXE)

%.o: $(SRCDIR)/%.c
	@echo "COMPILE $< to $@"
	@echo "FLAGS $(CFLAGS) -c"
	$(shell echo $@ >>.gitignore)
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo

.gitignore:
	$(shell echo .gitignore >$@)

.PHONY: all clean killd install

killd:
	@echo "killing daemon"
	-@$(shell sudo kill $(shell sudo cat /var/run/thinkd.pid))

clean:
	$(RM) $(OBJS) $(EXE) .gitignore

install: $(EXE)
	install -m 0755 $(EXE) $(BINDIR)
	install -m 0755 $(INITDIR)/$(EXE) /etc/init.d
	install -m 0644 thinkd.ini /etc
