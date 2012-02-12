# Compiler specific
CC = gcc
CFLAGS = -std=gnu99 -O2 -pedantic -Wall -g
CPPFLAGS = -DMAX_LOG_SIZE=262144
EXE = thinkd
SRCS = thinkd.c conf_utils.c acpi.c \
	   logger.c eclib.c 
OBJS = $(SRCS:.c=.o)

# Application directories
SRCDIR = src
INITDIR = init.d
REAL_SRCS = $(addprefix $(SRCDIR)/, $(SRCS))
MANPAGES = $(addsuffix .gz, $(addprefix man/, thinkd.8))

# Install dirs 
PREFIX ?= /usr/local
INST_INITDIR = /etc/init.d/
INST_CONFDIR = /etc
INST_BINDIR = $(PREFIX)/bin
INST_MANDIR = $(PREFIX)/man/man8
INST_SYSTEMD_DIR = $(PREFIX)/lib/systemd/system

# Commands
RM = rm -f
INSTALL = install
STRIP = strip --strip-all
GZIP = gzip

all: $(EXE) $(MANPAGES)

$(EXE): $(OBJS)
# @echo "LINK $(EXE) $(OBJS)"	
	$(LINK.c) -o $@ $(OBJS)
# @echo "STRIP $(EXE)"
	$(STRIP) $(EXE)

man/%.8.gz: man/%.8
	$(GZIP) $< -c > $@

$(OBJS): $(SRCDIR)/config.h \
			$(SRCDIR)/void.h

$(OBJS): %.o: $(SRCDIR)/%.c $(SRCDIR)/%.h
# @echo "CC $@"
	$(COMPILE.c) -o $@ $<

.PHONY: all clean killd install TAGS

TAGS:
	@printf "generating etags\n"
	$(shell cd $(SRCDIR) && etags *.c *.h)

killd:
	@printf "killing daemon\n"
	$(shell sudo kill -s SIGTERM $(shell sudo cat /var/run/thinkd.pid))

clean:
	$(RM) $(OBJS) $(EXE) $(MANPAGES)

install: $(EXE)
	install -m 0755 $(EXE) $(INST_BINDIR)
ifeq ($(shell uname -r | egrep -q "fc1[6-9]+" && echo 1),1)
	@echo "Detected fedora 16+"
	install -m 0755 systemd/$(EXE).service $(INST_SYSTEMD_DIR)
	systemctl enable $(EXE).service
else
	$(INSTALL) -m 0755 $(INITDIR)/$(EXE) $(INST_INITDIR)
	$(INSTALL) -m 0644 thinkd.ini $(INST_CONFDIR)
	$(INSTALL) -m 0644 $(MANPAGES) $(INST_MANDIR)
endif
