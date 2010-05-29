CPPFLAGS ?= -I/usr/local/include
CXXFLAGS ?= -g -O2
CXXFLAGS += -pedantic -Wall -Wno-long-long
LDFLAGS ?= -L/usr/local/lib
LDLIBS := -lkvm -lpopt

.PHONY: all man clean

all: dtpstree

man: man1/dtpstree.1

man1/%.1: %
	help2man -Nn '$(shell sed -e '$$ s|^// ||p;d' $<.cpp)' -o $@ $(shell realpath $<)

clean:
	rm -f dtpstree $(wildcard *core)
