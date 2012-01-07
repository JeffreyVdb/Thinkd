CC = gcc
CFLAGS = -std=gnu99 -O2 -pedantic -Wall -g
DEFINES = -DMAX_LOG_SIZE=262144
EXE = thinkd
SRCS = thinkd.c conf_utils.c acpi.c logger.c wrap.c

# use macro to replace the source suffixes to .o
OBJS = $(SRCS:.c=.o)
RM = rm -f
SRCDIR = src
INITDIR = init.d
REAL_SRCS = $(addprefix $(SRCDIR)/, $(SRCS))
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
SYSTEMD_DIR = /lib/systemd/system

all: $(EXE)

$(EXE): $(OBJS)
	@echo "LINK $(EXE) $(OBJS)"	
	@$(CC) $(CFLAGS) -o $@ $(OBJS)
	@echo "STRIP $(EXE)"
	@strip --strip-all $(EXE)

%.o: $(SRCDIR)/%.c
	@echo "CC $@"
	@$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

.PHONY: all clean killd install etags

etags:
	@echo "generating etags"
	-@$(shell cd $(SRCDIR) && etags *.c *.h)

killd:
	@echo "killing daemon"
	-@$(shell sudo kill -s SIGTERM $(shell sudo cat /var/run/thinkd.pid))

clean:
	$(RM) $(OBJS) $(EXE)

install: $(EXE)
	install -m 0755 $(EXE) $(BINDIR)
ifeq ($(shell uname -r | egrep -q "fc1[6-9]+" && echo 1),1)
	@echo "Detected fedora 16+"
	install -m 0755 systemd/$(EXE).service /lib/systemd/system
	systemctl enable $(EXE).service
else
	install -m 0755 $(INITDIR)/$(EXE) /etc/init.d
	install -m 0644 thinkd.ini /etc
endif
