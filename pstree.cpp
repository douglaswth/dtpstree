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

	std::setlocale(LC_ALL, "");

	char line[MB_LEN_MAX];
	int size(std::wctomb(line, L'└'));

	if (size != -1)
		std::cout << std::string(line, size) << std::endl;

	return 0;
}
