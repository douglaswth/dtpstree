# dtpstree

`dtpstree` shows running processes as a tree. It is a reimplementation of
`pstree` from [PSmisc] for [FreeBSD], [NetBSD], [OpenBSD], [DragonFly BSD], and
possibly other modern BSD variants. It also works without `/proc` and will show
the full set of processes in a jail even if `init` is not present.

It is released under the [Apache License, Version 2.0].

* [Download](#download)
  * [FreeBSD](#freebsd)
  * [pkgsrc (NetBSD)](#pkgsrc-netbsd)
  * [NetBSD](#netbsd)
  * [DragonFly BSD](#dragonfly-bsd)
  * [Debian GNU/kFreeBSD](#debian-gnukfreebsd)
* [Documentation](#documentation)

## Download

* [dtpstree-1.0.3.tar.bz2] 50.5 kB
* [dtpstree-1.0.3.tar.xz] 46.7 kB

You can check out the code straight from the [Git repository]:

```bash
git clone https://git.douglasthrift.net/douglas/dtpstree.git
```

The code is also available on [GitHub] if you would like to make a pull request
there.

### FreeBSD

On FreeBSD, you can install it from the port
[sysutils/dtpstree](https://svnweb.freebsd.org/ports/head/sysutils/dtpstree/):

```bash
cd /usr/ports/sysutils/dtpstree
make
make install
```

Or you can use `pkg` instead:

```bash
pkg install dtpstree
```

### pkgsrc (NetBSD)

On NetBSD or other systems using pkgsrc, you can install it from the port
[sysutils/dtpstree](http://cvsweb.netbsd.org/bsdweb.cgi/pkgsrc/sysutils/dtpstree/):

```bash
cd /usr/pkgsrc/sysutils/dtpstree
make
make install
```

Or, if there is a binary package, you can use `pkg_add` instead:

```bash
pkg_add dtpstree
```

### OpenBSD

On OpenBSD, you can install it from the port
[sysutils/dtpstree](http://cvsweb.openbsd.org/cgi-bin/cvsweb/ports/sysutils/dtpstree/):

```bash
cd /usr/ports/sysutils/dtpstree
make
make install
```

Or you can use `pkg_add` instead:

```bash
pkg_add dtpstree
```

### DragonFly BSD

On DragonFly BSD, you can install it from the port
[sysutils/dtpstree](https://github.com/DragonFlyBSD/DPorts/tree/master/sysutils/dtpstree):

```bash
cd /usr/dports/sysutils/dtpstree
make
make install
```

Or you can use `pkg` instead:

```bash
pkg install dtpstree
```

### Debian GNU/kFreeBSD

On [Debian GNU/kFreeBSD] you can install it [apt.douglasthrift.net]:

```bash
apt-get install dtpstree
```

## Documentation

```nohighlight
Usage: dtpstree [options] [PID|USER]

Options:
  -a, --arguments             show command line arguments
  -A, --ascii                 use ASCII line drawing characters
  -c, --no-compact            don't compact identical subtrees
  -h, --help                  show this help message and exit
  -H[PID], --highlight[=PID]  highlight the current process (or PID) and its
                              ancestors
  -G, --vt100                 use VT100 line drawing characters
  -k, --show-kernel           show kernel processes
  -l, --long                  don't truncate long lines
  -n, --numeric-sort          sort output by PID
  -p, --show-pids             show PIDs; implies -c
  -t, --show-titles           show process titles
  -u, --uid-changes           show uid transitions
  -U, --unicode               use Unicode line drawing characters
  -V, --version               show version information and exit
  PID, --pid=PID              show only the tree rooted at the process PID
  USER, --user=USER           show only trees rooted at processes of USER
```

[PSmisc]: https://psmisc.sourceforge.net/
[FreeBSD]: https://www.freebsd.org/
[NetBSD]: https://www.netbsd.org/
[OpenBSD]: https://www.openbsd.org/
[DragonFly BSD]: https://www.dragonflybsd.org/
[Apache License, Version 2.0]: https://www.apache.org/licenses/LICENSE-2.0
[dtpstree-1.0.3.tar.bz2]: https://dl.douglasthrift.net/dtpstree/dtpstree-1.0.3.tar.bz2
[dtpstree-1.0.3.tar.xz]: https://dl.douglasthrift.net/dtpstree/dtpstree-1.0.3.tar.xz
[Git repository]: https://git.douglasthrift.net/douglas/dtpstree
[GitHub]: https://github.com/douglaswth/dtpstree
[Debian GNU/kFreeBSD]: https://www.debian.org/ports/kfreebsd-gnu/
[apt.douglasthrift.net]: https://apt.douglasthrift.net/
