# libuwifi - Userspace Wifi Library
#
# Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
#
# This program is licensed under the GNU Lesser General Public License,
# Version 3. See the file COPYING for more details.

NAME = libuwifi

# build options
DEBUG = 0
PLATFORM = linux

OBJS += core/channel.o
OBJS += core/inject.o
OBJS += core/node.o
OBJS += core/wlan_parser.o
OBJS += core/wlan_util.o
OBJS += core/essid.o
OBJS += util/average.o
OBJS += util/util.o

INCLUDES = -I. -I./core -I./util -I./$(PLATFORM)
CFLAGS += -std=gnu99 -Wall -Wextra $(INCLUDES) -DDEBUG=$(DEBUG) -DUWIFI_VER=\"$(shell git describe --tags)\"

include $(PLATFORM)/platform.mk

.PHONY: all check clean force

.objdeps.mk: $(OBJS:%.o=%.c)
	gcc -MM $(INCLUDES) $^ >$@

-include .objdeps.mk

$(NAME).a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

$(OBJS): .buildflags

check: $(OBJS:%.o=%.c)
	sparse $(CFLAGS) $^

clean:
	-rm -f core/*.o util/*.o linux/*.o osx/*.o esp8266/*.o *~
	-rm -f $(NAME).so*
	-rm -f $(NAME).a*
	-rm -f .buildflags
	-rm -f .objdeps.mk
	-rm -f radiotap/*.o

.buildflags: force
	echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@
