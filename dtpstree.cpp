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
#include <paths.h>
#include <popt.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#include "foreach.hpp"

class Proc;

typedef std::map<pid_t, Proc *> PidMap;
typedef std::multimap<std::string, Proc *> NameMap;

enum Sort { Pid, Name };

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
	poptOption options[] = {
		{ "arguments", 'a', POPT_ARG_NONE, NULL, 0, "show command line arguments", NULL },
		{ "ascii", 'A', POPT_ARG_NONE, NULL, 0, "use ASCII line drawing characters", NULL },
		{ "compact", 'c', POPT_ARG_NONE, NULL, 0, "don't compact identical subtrees", NULL },
		{ "highlight-all", 'h', POPT_ARG_NONE, NULL, 0, "highlight current process and its ancestors", NULL },
		{ "highlight-pid", 'H', POPT_ARG_INT, NULL, 0, "highlight this process and its ancestors", "PID" },
		{ "vt100", 'G', POPT_ARG_NONE, NULL, 0, "use VT100 line drawing characters", NULL },
		{ "long", 'l', POPT_ARG_NONE, NULL, 0, "don't truncate long lines", NULL },
		{ "numeric-sort", 'n', POPT_ARG_NONE, NULL, 0, "sort output by PID", NULL },
		{ "show-pids", 'p', POPT_ARG_NONE, NULL, 0, "show PIDs; implies -c", NULL },
		{ "uid-changes", 'u', POPT_ARG_NONE, NULL, 0, "show uid transitions", NULL },
		{ "unicode", 'U', POPT_ARG_NONE, NULL, 0, "use Unicode line drawing characters", NULL },
		{ "version", 'V', POPT_ARG_NONE, NULL, 0, "display version information", NULL },
		POPT_AUTOHELP
		POPT_TABLEEND
	};
	poptContext context(poptGetContext(NULL, argc, const_cast<const char **>(argv), options, 0));

	poptSetOtherOptionHelp(context, "[OPTION...] [PID|USER]");

	std::cout << poptGetNextOpt(context) << std::endl;

	poptFreeContext(context);

	return 0;

	char error[_POSIX2_LINE_MAX];
	kvm_t *kd(kvm_openfiles(NULL, _PATH_DEVNULL, NULL, O_RDONLY, error));

	if (!kd)
		errx(1, "%s", error);

	int count;
	InfoProc procs(kvm_getprocs(kd, KERN_PROC_PROC, 0, &count));

	if (!procs)
		errx(1, "%s", kvm_geterr(kd));

	PidMap pids;

	_forall (InfoProc, proc, procs, procs + count)
		if (proc->ki_ppid != 0 || proc->ki_pid == 1)
			pids[proc->ki_pid] = new Proc(kd, proc);

	Sort sort(Name);

	_foreach (PidMap, pid, pids)
	{
		Proc *proc(pid->second);
		PidMap::iterator parent(pids.find(proc->parent()));

		if (parent != pids.end())
			parent->second->child(proc);
	}

	if (true)
	{
		pid_t hpid(0);
		PidMap::iterator pid(pids.find(hpid ? hpid : getpid()));

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

	return 0;
}
