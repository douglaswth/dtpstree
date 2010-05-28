// DT PS Tree
//
// Douglas Thrift
//
// $Id$

#include <climits>
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <libgen.h>
#include <paths.h>
#include <popt.h>
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
	None		= 0x000,
	Arguments	= 0x001,
	Ascii		= 0x002,
	Compact		= 0x004,
	Vt100		= 0x008,
	ShowKernel	= 0x010,
	Long		= 0x020,
	NumericSort	= 0x040,
	ShowPids	= 0x084,
	UidChanges	= 0x100,
	Unicode		= 0x200,
	Version		= 0x400
};

class Proc
{
	kvm_t *kd_;
	kinfo_proc *proc_;
	Proc *parent_;
	PidMap childrenByPid_;
	NameMap childrenByName_;
	bool highlight_;

public:
	Proc() {}
	Proc(kvm_t *kd, kinfo_proc *proc) : kd_(kd), proc_(proc) {}

	inline std::string name() const { return proc_->ki_comm; }
	inline pid_t parent() const { return proc_->ki_ppid; }
	inline pid_t pid() const { return proc_->ki_pid; }

	void child(Proc *proc)
	{
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

		print << name();

		bool _pid(true), _args(true);
		bool change(true && parent_ && uid() != parent_->uid());
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
				if (true && (indent += 1 + std::strlen(*argv)) > 75)
				{
					args << " ...";

					break;
				}

				args << ' ' << *argv;
			}

		return args.str();
	}
};

typedef kinfo_proc *InfoProc;

int main(int argc, char *argv[])
{
	int flags(0);
	pid_t hpid(-2);
	poptOption options[] = {
		{ "arguments", 'a', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags, Arguments, "show command line arguments", NULL },
		{ "ascii", 'A', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags, Ascii, "use ASCII line drawing characters", NULL },
		{ "compact", 'c', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags, Compact, "don't compact identical subtrees", NULL },
		{ "highlight-all", 'h', POPT_ARG_VAL, &hpid, -1, "highlight current process and its ancestors", NULL },
		{ "highlight-pid", 'H', POPT_ARG_INT, &hpid, 0, "highlight this process and its ancestors", "PID" },
		{ "vt100", 'G', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags, Vt100, "use VT100 line drawing characters", NULL },
		{ "show-kernel", 'k', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags, ShowKernel, "show kernel processes", NULL },
		{ "long", 'l', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags, Long, "don't truncate long lines", NULL },
		{ "numeric-sort", 'n', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags, NumericSort, "sort output by PID", NULL },
		{ "show-pids", 'p', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags, ShowPids, "show PIDs; implies -c", NULL },
		{ "uid-changes", 'u', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags, UidChanges, "show uid transitions", NULL },
		{ "unicode", 'U', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags, Unicode, "use Unicode line drawing characters", NULL },
		{ "version", 'V', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags, Version, "display version information", NULL },
		{ "pid", '\0', POPT_ARG_INT, NULL, 0, "start at this PID", "PID" },
		{ "user", '\0', POPT_ARG_STRING, NULL, 0, "show only trees rooted at processes of this user", "USER" },
		POPT_AUTOHELP
		POPT_TABLEEND
	};
	poptContext context(poptGetContext(NULL, argc, const_cast<const char **>(argv), options, 0));

	poptSetOtherOptionHelp(context, "[OPTION...] [PID|USER]");

	int count;

	if ((count = poptGetNextOpt(context)) < -1)
	{
		std::cerr << basename(argv[0]) << ": " << poptStrerror(count) << ": " << poptBadOption(context, 0) << std::endl;

		return 1;
	}

	poptFreeContext(context);

	if (flags & Version)
	{
		std::cout << DTPSTREE_PROGRAM " " DTPSTREE_VERSION << std::endl;

		return 0;
	}

	char error[_POSIX2_LINE_MAX];
	kvm_t *kd(kvm_openfiles(NULL, _PATH_DEVNULL, NULL, O_RDONLY, error));

	if (!kd)
		errx(1, "%s", error);

	InfoProc procs(kvm_getprocs(kd, KERN_PROC_PROC, 0, &count));

	if (!procs)
		errx(1, "%s", kvm_geterr(kd));

	PidMap pids;

	_forall (InfoProc, proc, procs, procs + count)
		if (proc->ki_ppid != 0 || proc->ki_pid == 1)
			pids[proc->ki_pid] = new Proc(kd, proc);

	enum { Pid, Name } sort(flags & NumericSort ? Pid : Name);

	_foreach (PidMap, pid, pids)
	{
		Proc *proc(pid->second);
		PidMap::iterator parent(pids.find(proc->parent()));

		if (parent != pids.end())
			parent->second->child(proc);
	}

	if (hpid != -2)
	{
		PidMap::iterator pid(pids.find(hpid != -1 ? hpid : getpid()));

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
			case Pid:
				proc->printByPid();
				break;
			case Name:
				names.insert(NameMap::value_type(proc->name(), proc));
			}
	}

	switch (sort)
	{
	case Name:
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
