# DT PS Tree
#
# Douglas Thrift
#
# GNUmakefile

#  Copyright 2010 Douglas Thrift
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

CXXFLAGS ?= -g -O2
override CXXFLAGS += -Wall -Wno-long-long -Wno-parentheses
LDLIBS := -lkvm -lncurses
TARNAME := $(shell sed -e 's/^\#define DTPSTREE_PROGRAM "\(.*\)"$$/\1/p;d' dtpstree.cpp)-$(shell sed -e 's/^\#define DTPSTREE_VERSION "\(.*\)"$$/\1/p;d' dtpstree.cpp)
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/man
MAN1DIR ?= $(MANDIR)/man1

.PHONY: all man dist install uninstall clean

all: dtpstree

man: man1/dtpstree.1

man1/%.1: % %.1.in
	help2man -I $*.1.in -Nn '$(shell sed -e '$$ s|^// ||p;d' $<.cpp)' -o $@ $(shell realpath $<)

dist: man
	bsdtar -cf $(TARNAME).tar.bz2 -js '/^\./$(TARNAME)/' -vX .gitignore --exclude='.git*' .

install: all
	install -dv $(DESTDIR)$(BINDIR) $(DESTDIR)$(MAN1DIR)
	install -cv dtpstree $(DESTDIR)$(BINDIR)
	install -cm 644 -v man1/dtpstree.1 $(DESTDIR)$(MAN1DIR)

uninstall:
	rm -fv $(DESTDIR)$(BINDIR)/dtpstree
	rm -fv $(DESTDIR)$(MAN1DIR)/dtpstree.1

clean:
	rm -fv dtpstree $(wildcard *core)
