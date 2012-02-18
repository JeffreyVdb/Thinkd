# Compiler specific
CC 			:= gcc
CFLAGS 		:= -std=gnu99 -O2 -pedantic -Wall -g -fomit-frame-pointer
CPPFLAGS 	:= -DMAX_LOG_SIZE=262144
EXE  		:= thinkd
SRCS  		:= thinkd.c conf_utils.c acpi.c \
	   			logger.c eclib.c 
OBJS 		:= $(addprefix obj/, $(SRCS:.c=.o))

# Application directories
SRCDIR 		:= src
OBJDIR		:= obj
VPATH 		:= $(SRCDIR) 
INITDIR 	:= init.d
MANDIR		:= man
MANPAGES 	:= $(addsuffix .gz, $(addprefix man/, thinkd.8))

# Install dirs 
PREFIX 			?= /usr/local
INST_INITDIR 	:= /etc/init.d/
INST_CONFDIR 	:= /etc
INST_BINDIR 	:= $(PREFIX)/bin
INST_MANDIR 	:= $(PREFIX)/man/man8
INST_SYSTEMD_DIR := $(PREFIX)/lib/systemd/system

# Commands
RM = rm -f
INSTALL = install
STRIP = strip --strip-all
GZIP = gzip
MKDIR = mkdir -p

# Other
Q ?= @
ifneq ($(Q), @)
override Q := 
endif

all: $(EXE) $(MANPAGES)

$(EXE): $(OBJS)
ifeq ($(Q), @)
	@printf "LINK $(EXE) $(OBJS)\n"
	@printf "STRIP $(EXE)\n"
endif
	$(Q)$(LINK.o) -o $@ $(OBJS)
	$(Q)$(STRIP) $(EXE)

$(MANDIR)/%.8.gz: $(MANDIR)/%.8
ifeq ($(Q), @)
	@printf "COMPRESS $^\n"
endif
	$(Q)$(GZIP) $< -c > $@

$(OBJDIR):
	$(MKDIR) $(OBJDIR)

$(OBJS): | $(OBJDIR)
$(OBJS): config.h \
			void.h

$(OBJS): $(OBJDIR)/%.o: %.c %.h
ifeq ($(Q), @)
	@printf	"CC $@\n"
endif
	$(Q)$(COMPILE.c) -o $@ $<

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
