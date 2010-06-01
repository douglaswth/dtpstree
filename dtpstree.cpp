// DT PS Tree
//
// Douglas Thrift
//
// dtpstree.cpp

/*  Copyright 2010 Douglas Thrift
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

#ifndef __GLIBC__
#include <libgen.h>
#endif

#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <kvm.h>
#include <paths.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#include "foreach.hpp"

#define DTPSTREE_PROGRAM "dtpstree"
#define DTPSTREE_VERSION "1.0.1"

class Proc;

typedef std::map<pid_t, Proc *> PidMap;
typedef std::multimap<std::string, Proc *> NameMap;

enum Flags
{
	Arguments	= 0x0001,
	Ascii		= 0x0002,
	Compact		= 0x0004,
	Highlight	= 0x0008,
	Vt100		= 0x0010,
	ShowKernel	= 0x0020,
	Long		= 0x0040,
	NumericSort	= 0x0080,
	ShowPids	= 0x0104,
	ShowTitles	= 0x0200,
	UidChanges	= 0x0400,
	Unicode		= 0x0800,
	Version		= 0x1000,
	Pid			= 0x2000,
	User		= 0x4000
};

class Proc
{
	const int &flags_;
	kvm_t *kd_;
	kinfo_proc *proc_;
	Proc *parent_;
	PidMap childrenByPid_;
	NameMap childrenByName_;
	bool highlight_;

public:
	Proc(const int &flags, kvm_t *kd, kinfo_proc *proc) : flags_(flags), kd_(kd), proc_(proc) {}

	inline std::string name() const { return proc_->ki_comm; }
	inline pid_t parent() const { return proc_->ki_ppid; }
	inline pid_t pid() const { return proc_->ki_pid; }

	void child(Proc *proc)
	{
		if (proc == this)
			return;

		proc->parent_ = this;
		childrenByPid_[proc->pid()] = proc;

		childrenByName_.insert(NameMap::value_type(proc->name(), proc));
	}

	void highlight()
	{
		highlight_ = true;

		if (parent_)
			parent_->highlight();
	}

	inline bool root() const { return !parent_; }

	void printByPid(const std::string &indent = "") const
	{
		std::cout << indent << print(indent.size()) << std::endl;

		_foreach (const PidMap, child, childrenByPid_)
			child->second->printByPid(indent + "  ");
	}

	void printByName(const std::string &indent = "") const
	{
		std::cout << indent << print(indent.size()) << std::endl;

		_foreach (const NameMap, child, childrenByName_)
			child->second->printByName(indent + "  ");
	}

private:
	std::string print(std::string::size_type indent) const
	{
		std::ostringstream print;

		if (highlight_)
			print << "\033[1m";

		if (flags_ & ShowTitles)
		{
			char **argv(kvm_getargv(kd_, proc_, 0));

			if (argv)
				print << *argv;
			else
				print << name();
		}
		else
			print << name();

		bool _pid(flags_ & ShowPids), _args(flags_ & Arguments);
		bool change(flags_ & UidChanges && parent_ && uid() != parent_->uid());
		bool parens((_pid || change) && !_args);

		if (parens)
			print << '(';

		if (_pid)
		{
			if (!parens)
				print << ',';

			print << pid();
		}

		if (change)
		{
			if (!parens || _pid)
				print << ',';

			print << user();
		}

		if (parens)
			print << ')';

		if (highlight_)
			print << "\033[22m";

		if (_args)
			print << args(indent + print.str().size());

		return print.str();
	}

	inline bool children() const { return childrenByPid_.size(); }

	inline uid_t uid() const { return proc_->ki_ruid; }

	std::string user() const
	{
		passwd *user(getpwuid(uid()));

		return user->pw_name;
	}

	std::string args(std::string::size_type indent) const
	{
		char **argv(kvm_getargv(kd_, proc_, 0));
		std::ostringstream args;

		if (argv && *argv)
			for (++argv; *argv; ++argv)
			{
				if (!(flags_ & Long) && (indent += 1 + std::strlen(*argv)) > 75)
				{
					args << " ...";

					break;
				}

				args << ' ' << *argv;
			}

		return args.str();
	}
};

static void help(const char *program, option options[], int code = 0)
{
	std::cout << "Usage: " << basename(program) << " [options] [PID|USER]" << std::endl << std::endl << "Options:" << std::endl;

	for (option *option(options); option->name; ++option)
	{
		std::string name(option->name);
		std::ostringstream arguments;

		switch (option->val)
		{
		case 'H':
			if (name != "highlight")
				continue;

			arguments << "-H[PID], --highlight[=PID]"; break;
		case 0:
			if (name == "pid")
				arguments << "PID, --pid=PID";
			else if (name == "user")
				arguments << "USER, --user=USER";
			else
				goto argument;

			break;
		default:
			arguments << '-' << static_cast<char>(option->val) << ", ";
		argument:
			arguments << "--" << name;
		}

		const char *description("");

		switch (option->val)
		{
		case 'a':
			description = "show command line arguments"; break;
		case 'A':
			description = "use ASCII line drawing characters"; break;
		case 'c':
			description = "don't compact identical subtrees"; break;
		case 'h':
			description = "show this help message and exit"; break;
		case 'H':
			description = "highlight the current process (or PID) and its\n                              ancestors"; break;
		case 'G':
			description = "use VT100 line drawing characters"; break;
		case 'k':
			description = "show kernel processes"; break;
		case 'l':
			description = "don't truncate long lines"; break;
		case 'n':
			description = "sort output by PID"; break;
		case 'p':
			description = "show PIDs; implies -c"; break;
		case 't':
			description = "show process titles"; break;
		case 'u':
			description = "show uid transitions"; break;
		case 'U':
			description = "use Unicode line drawing characters"; break;
		case 'V':
			description = "show version information and exit"; break;
		case 0:
			if (name == "pid")
				description = "show only the tree roted at the process PID";
			else if (name == "user")
				description = "show only trees rooted at processes of USER";
		}

		std::printf("  %-27s %s\n", arguments.str().c_str(), description);
	}

	std::exit(code);
}

template <typename Type, long minimum, long maximum>
static Type value(char *program, option options[], bool *success = NULL)
{
	char *end;
	long value(std::strtol(optarg, &end, 0));

	if (optarg == end || *end != '\0')
		if (success)
			*success = false;
		else
		{
			warnx("invalid numeric value: \"%s\"", optarg);
			help(program, options, 1);
		}
	else if (value < minimum)
	{
		warnx("number too small: %s", optarg);
		help(program, options, 1);
	}
	else if (value > maximum)
	{
		warnx("number too large: %s", optarg);
		help(program, options, 1);
	}
	else if (success)
		*success = true;

	return value;
}

static int options(int argc, char *argv[], pid_t &hpid, pid_t &pid, char *&user)
{
	option options[] = {
		{ "arguments", no_argument, NULL, 'a' },
		{ "ascii", no_argument, NULL, 'A' },
		{ "compact", no_argument, NULL, 'c' },
		{ "help", no_argument, NULL, 'h' },
		{ "highlight", optional_argument, NULL, 'H' },
		{ "highlight-all", no_argument, NULL, 'H' },
		{ "highlight-pid", required_argument, NULL, 'H' },
		{ "vt100", no_argument, NULL, 'G' },
		{ "show-kernel", no_argument, NULL, 'k' },
		{ "long", no_argument, NULL, 'l' },
		{ "numeric-sort", no_argument, NULL, 'n' },
		{ "show-pids", no_argument, NULL, 'p' },
		{ "show-titles", no_argument, NULL, 't' },
		{ "uid-changes", no_argument, NULL, 'u' },
		{ "unicode", no_argument, NULL, 'U' },
		{ "version", no_argument, NULL, 'V' },
		{ "pid", required_argument, NULL, 0 },
		{ "user", required_argument, NULL, 0 },
		{ NULL, 0, NULL, 0 }
	};
	int option, index, flags(0);
	char *program(argv[0]);

	while ((option = getopt_long(argc, argv, "aAchH::GklnptuUV", options, &index)) != -1)
		switch (option)
		{
		case 'a':
			flags |= Arguments; break;
		case 'A':
			flags |= Ascii;
			flags &= ~Vt100;
			flags &= ~Unicode;

			break;
		case 'c':
			flags |= Compact; break;
		case 'h':
			help(program, options);
		case 'H':
			hpid = optarg ? value<pid_t, 0, INT_MAX>(program, options) : getpid();
			flags |= Highlight;

			break;
		case 'G':
			flags |= Vt100;
			flags &= ~Ascii;
			flags &= ~Unicode;

			break;
		case 'k':
			flags |= ShowKernel; break;
		case 'l':
			flags |= Long; break;
		case 'n':
			flags |= NumericSort; break;
		case 'p':
			flags |= ShowPids; break;
		case 't':
			flags |= ShowTitles; break;
		case 'u':
			flags |= UidChanges; break;
		case 'U':
			flags |= Unicode;
			flags &= ~Ascii;
			flags &= ~Vt100;

			break;
		case 'V':
			flags |= Version; break;
		case 0:
			{
				std::string option(options[index].name);

				if (option == "pid")
				{
					pid = value<pid_t, 0, INT_MAX>(program, options);
					flags |= Pid;
					flags &= ~User;
				}
				else if (option == "user")
				{
					std::free(user);

					user = strdup(optarg);
					flags |= User;
					flags &= ~Pid;
				}
			}

			break;
		case '?':
			help(program, options, 1);
		}

	_forall (int, index, optind, argc)
	{
		bool success;

		optarg = argv[index];
		pid = value<pid_t, 0, INT_MAX>(program, options, &success);

		if (success)
		{
			std::free(user);

			user = strdup(optarg);
			flags |= User;
			flags &= ~Pid;
		}
		else
		{
			flags |= Pid;
			flags &= ~User;
		}
	}

	return flags;
}

int main(int argc, char *argv[])
{
	pid_t hpid, pid;
	char *user(NULL);
	int flags(options(argc, argv, hpid, pid, user));

	if (flags & Version)
	{
		std::cout << DTPSTREE_PROGRAM " " DTPSTREE_VERSION << std::endl;

		return 0;
	}

	char error[_POSIX2_LINE_MAX];
	kvm_t *kd(kvm_openfiles(NULL, _PATH_DEVNULL, NULL, O_RDONLY, error));

	if (!kd)
		errx(1, "%s", error);

	typedef kinfo_proc *InfoProc;

	int count;
	InfoProc procs(kvm_getprocs(kd, KERN_PROC_PROC, 0, &count));

	if (!procs)
		errx(1, "%s", kvm_geterr(kd));

	PidMap pids;

	_forall (InfoProc, proc, procs, procs + count)
		if (flags & ShowKernel || proc->ki_ppid != 0 || proc->ki_pid == 1)
			pids[proc->ki_pid] = new Proc(flags, kd, proc);

	enum { PidSort, NameSort } sort(flags & NumericSort ? PidSort : NameSort);

	_foreach (PidMap, pid, pids)
	{
		Proc *proc(pid->second);
		PidMap::iterator parent(pids.find(proc->parent()));

		if (parent != pids.end())
			parent->second->child(proc);
	}

	if (flags & Highlight)
	{
		PidMap::iterator pid(pids.find(hpid));

		if (pid != pids.end())
			pid->second->highlight();
	}

	NameMap names;

	_foreach (PidMap, pid, pids)
	{
		Proc *proc(pid->second);

		if (proc->root())
			switch (sort)
			{
			case PidSort:
				proc->printByPid();

				break;
			case NameSort:
				names.insert(NameMap::value_type(proc->name(), proc));
			}
	}

	switch (sort)
	{
	case NameSort:
		_foreach (NameMap, name, names)
			name->second->printByName();
	default:
		break;
	}

	std::setlocale(LC_ALL, "");

	char line[MB_LEN_MAX];
	int size(std::wctomb(line, L'└'));

	if (size != -1)
		std::cout << std::string(line, size) << std::endl;

	return 0;
}

// display a tree of processes
