CPPFLAGS := -I/usr/local/include
CXXFLAGS := -pedantic -Wall -O2 -Wno-long-long
LDFLAGS := -L/usr/local/lib
LDLIBS := -lkvm -lpopt

.PHONY: all man clean

all: dtpstree

man: man1/dtpstree.1

man1/%.1: %
	help2man -Nn '$(shell sed -e '$$ s/^# //p;d' $<)' -o $@ $(shell realpath $<)

clean:
	rm -f dtpstree
