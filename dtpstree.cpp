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

#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifdef __GLIBC__
#include <bsd/stdlib.h>
#else
#include <libgen.h>
#endif

#include <curses.h>
#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <kvm.h>
#include <paths.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <term.h>
#include <vis.h>

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
	ShowPids	= 0x0100,
	ShowTitles	= 0x0200,
	UidChanges	= 0x0400,
	Unicode		= 0x0800,
	Version		= 0x1000,
	Pid			= 0x2000,
	User		= 0x4000
};

enum Escape { None, BoxDrawing, Bright };

struct Segment
{
	size_t width_;
	Escape escape_;
	char *string_;

	inline Segment(size_t width, Escape escape, char *string) : width_(width), escape_(escape), string_(string) {}
};

struct Branch
{
	std::string indentation_;
	bool done_;

	inline Branch(size_t indentation) : indentation_(indentation, ' '), done_(false) {}
};

class Tree
{
	const uint16_t &flags_;
	bool vt100_;
	wchar_t horizontal_, vertical_, upAndRight_, verticalAndRight_, downAndHorizontal_;
	size_t maxWidth_, width_;
	bool max_, suppress_;
	std::vector<Segment> segments_;
	std::vector<Branch> branches_;
	bool first_, last_;

public:
	Tree(const uint16_t &flags) : flags_(flags), vt100_(false), maxWidth_(0), width_(0), max_(false), suppress_(false)
	{
		bool tty(isatty(1));

		if (flags & Ascii)
		{
		ascii:
			horizontal_ = L'-';
			vertical_ = L'|';
			upAndRight_ = L'`';
			verticalAndRight_ = L'|';
			downAndHorizontal_ = L'+';
		}
		else if (flags & Unicode)
		{
		unicode:
			if (!std::setlocale(LC_CTYPE, ""))
				goto vt100;

			horizontal_ = L'\x2500';
			vertical_ = L'\x2502';
			upAndRight_ = L'\x2514';
			verticalAndRight_ = L'\x251c';
			downAndHorizontal_ = L'\x252c';

			char *test;

			if (asprintf(&test, "%lc%lc%lc%lc%lc", horizontal_, vertical_, upAndRight_, verticalAndRight_, downAndHorizontal_) == -1)
				goto vt100;

			std::free(test);
		}
		else if (flags & Vt100)
		{
		vt100:
			vt100_ = true;
			horizontal_ = L'\x71';
			vertical_ = L'\x78';
			upAndRight_ = L'\x6d';
			verticalAndRight_ = L'\x74';
			downAndHorizontal_ = L'\x77';
		}
		else if (tty)
			goto unicode;
		else
			goto ascii;

		if (!(flags & Long) && tty)
		{
			int code;

			if (setupterm(NULL, 1, &code) == OK)
			{
				maxWidth_ = tigetnum("cols");

				if (tigetflag("am") && !tigetflag("xenl"))
					suppress_ = true;
			}
			else
				maxWidth_ = 80;
		}
	}

	void print(const std::string &string, bool highlight)
	{
		Escape escape(vt100_ ? BoxDrawing : None);

		if (!first_ || flags_ & Arguments)
		{
			size_t last(branches_.size() - 1);

			_foreach (std::vector<Branch>, branch, branches_)
			{
				size_t width(branch->indentation_.size() + 2);

				if (_index == last)
				{
					wchar_t line;

					if (last_)
					{
						branch->done_ = true;
						line = upAndRight_;
					}
					else
						line = verticalAndRight_;

					print(width, escape, "%s%lc%lc", branch->indentation_.c_str(), line, horizontal_);
				}
				else
					print(width, escape, "%s%lc ", branch->indentation_.c_str(), branch->done_ ? ' ' : vertical_);
			}
		}
		else if (branches_.size())
		{
			wchar_t line;

			if (last_)
			{
				branches_.back().done_ = true;
				line = horizontal_;
			}
			else
				line = downAndHorizontal_;

			print(3, escape, "%lc%lc%lc", horizontal_, line, horizontal_);
		}

		print(string.size(), highlight ? Bright : None, "%s", string.c_str());

		branches_.push_back(Branch(!(flags_ & Arguments) ? string.size() + 1 : 2));
	}

	inline void printArg(const std::string &arg, bool last)
	{
		if (max_)
			return;

		size_t width(arg.size() + 1);

		width_ += width;

		char *string;

		if (maxWidth_ && !(flags_ & Long))
			if (width_ > maxWidth_ || !last && width_ + 3 >= maxWidth_)
			{
				width -= width_ - maxWidth_;
				width_ = maxWidth_;
				max_ = true;

				ssize_t size(static_cast<ssize_t>(width) - 4);

				asprintf(&string, " %s...", size > 0 ? arg.substr(0, size).c_str() : "");
			}
			else
				goto print;
		else
		print:
			asprintf(&string, " %s", arg.c_str());

		segments_.push_back(Segment(width, None, string));
	}

	inline void pop(bool children)
	{
		branches_.pop_back();

		if (!(flags_ & Arguments) && !children)
			done();
	}

	void done()
	{
		size_t last(segments_.size() - 1);

		_foreach (std::vector<Segment>, segment, segments_)
		{
			const char *begin, *end;

			switch (segment->escape_)
			{
			case BoxDrawing:
				begin = !_index || (segment - 1)->escape_ != BoxDrawing ? "\033(0\017" : "";
				end = _index == last || (segment + 1)->escape_ != BoxDrawing ? "\033(B\017" : "";
				break;
			case Bright:
				begin = "\033[1m";
				end = "\033[22m";
				break;
			default:
				begin = end = ""; break;
			}

			std::printf("%s%s%s", begin, segment->string_, end);
			std::free(segment->string_);
		}

		segments_.clear();

		if (suppress_ && width_ == maxWidth_)
			std::fflush(stdout);
		else
			std::printf("\n");

		width_ = 0;
		max_ = false;
	}

	inline Tree &operator()(bool first, bool last)
	{
		first_ = first;
		last_ = last;

		return *this;
	}

private:
	void print(size_t width, Escape escape, const char * format, ...)
	{
		if (max_)
			return;

		std::va_list args;

		va_start(args, format);

		char *string;

		vasprintf(&string, format, args);
		va_end(args);

		width_ += width;

		if (maxWidth_ && !(flags_ & Long))
			if (width_ > maxWidth_)
			{
				width -= width_ - maxWidth_;
				width_ = maxWidth_;
				max_ = true;

				bool previous = !width;

				if (previous)
				{
					std::free(string);

					const Segment &segment(segments_.back());

					width = segment.width_;
					string = segment.string_;
				}

				std::wstring wide(width - 1, '\0');

				std::mbstowcs(const_cast<wchar_t *>(wide.data()), string, wide.size());
				std::free(string);
				asprintf(&string, "%ls+", wide.c_str());

				if (previous)
				{
					segments_.back().string_ = string;

					return;
				}
			}

		segments_.push_back(Segment(width, escape, string));
	}
};

class Proc
{
	const uint16_t &flags_;
	kvm_t *kd_;
	kinfo_proc *proc_;
	mutable std::string name_;
	Proc *parent_;
	PidMap childrenByPid_;
	NameMap childrenByName_;
	bool highlight_, root_;

public:
	inline Proc(const uint16_t &flags, kvm_t *kd, kinfo_proc *proc) : flags_(flags), kd_(kd), proc_(proc), parent_(NULL), highlight_(false), root_(false) {}

	inline std::string name() const
	{
		if (name_.empty())
			name_ = visual(proc_->ki_comm);

		return name_;
	}

	inline pid_t parent() const { return proc_->ki_ppid; }
	inline pid_t pid() const { return proc_->ki_pid; }

	inline void child(Proc *proc)
	{
		if (proc == this)
			return;

		proc->parent_ = this;
		childrenByPid_[proc->pid()] = proc;

		childrenByName_.insert(NameMap::value_type(proc->name(), proc));
	}

	inline void highlight()
	{
		highlight_ = true;

		if (parent_)
			parent_->highlight();
	}

	inline bool root(uid_t uid)
	{
		if (flags_ & User)
		{
			if (uid == this->uid())
			{
				Proc *parent(parent_);

				while (parent)
				{
					if (parent->uid() == uid)
						return false;

					parent = parent->parent_;
				}

				return root_ = true;
			}

			return false;
		}

		return root_ = !parent_;
	}

	void printByPid(Tree &tree) const
	{
		print(tree);

		size_t last(childrenByPid_.size() - 1);

		_foreach (const PidMap, child, childrenByPid_)
			child->second->printByPid(tree(!_index, _index == last));

		tree.pop(children());
	}

	void printByName(Tree &tree) const
	{
		print(tree);

		size_t last(childrenByName_.size() - 1);

		_foreach (const NameMap, child, childrenByName_)
			child->second->printByName(tree(!_index, _index == last));

		tree.pop(children());
	}

private:
	inline std::string visual(const char *string) const
	{
		std::string visual(std::strlen(string) * 4 + 1, '\0');

		visual.resize(strvis(const_cast<char *>(visual.data()), string, VIS_TAB | VIS_NL | VIS_NOSLASH));

		return visual;
	}

	void print(Tree &tree) const
	{
		bool titles(flags_ & ShowTitles);
		char **argv(NULL);
		std::ostringstream print;

		if (titles)
		{
			argv = kvm_getargv(kd_, proc_, 0);

			if (argv)
				print << visual(*argv);
			else
				print << name();
		}
		else
			print << name();

		bool _pid(flags_ & ShowPids), args(flags_ & Arguments);
		bool change(flags_ & UidChanges && (root_ ? !(flags_ & User) && uid() : parent_ && uid() != parent_->uid()));
		bool parens((_pid || change) && !args);

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

			passwd *user(getpwuid(uid()));

			print << user->pw_name;
		}

		if (parens)
			print << ')';

		tree.print(print.str(), highlight_);

		if (args)
		{
			if (!titles && !argv)
				argv = kvm_getargv(kd_, proc_, 0);

			if (argv && *argv)
				for (++argv; *argv; ++argv)
					tree.printArg(visual(*argv), !*(argv + 1));

			tree.done();
		}
	}

	inline bool children() const { return childrenByPid_.size(); }

	inline uid_t uid() const { return proc_->ki_ruid; }
};

static void help(const char *program, option options[], int code = 0)
{
	std::printf("Usage: %s [options] [PID|USER]\n\nOptions:\n", basename(program));

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
				description = "show only the tree rooted at the process PID";
			else if (name == "user")
				description = "show only trees rooted at processes of USER";
		}

		std::printf("  %-27s %s\n", arguments.str().c_str(), description);
	}

	std::exit(code);
}

template <typename Type, long long minimum, long long maximum>
static Type value(char *program, option options[], bool *success = NULL)
{
	const char *error;
	long long value(strtonum(optarg, minimum, maximum, &error));

	if (error)
		if (success && errno == EINVAL)
			*success = false;
		else
		{
			warnx("Number is %s: \"%s\"", error, optarg);
			help(program, options, 1);
		}
	else if (success)
		*success = true;

	return value;
}

static uint16_t options(int argc, char *argv[], pid_t &hpid, pid_t &pid, char *&user)
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
	int option, index;
	uint16_t flags(0);
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
			flags |= Compact | ShowPids; break;
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
			flags |= Pid;
			flags &= ~User;
		}
		else
		{
			std::free(user);

			user = strdup(optarg);
			flags |= User;
			flags &= ~Pid;
		}
	}

	return flags;
}

int main(int argc, char *argv[])
{
	pid_t hpid(0), pid(0);
	char *user(NULL);
	uint16_t flags(options(argc, argv, hpid, pid, user));

	if (flags & Version)
	{
		std::printf(DTPSTREE_PROGRAM " " DTPSTREE_VERSION "\n");

		return 0;
	}

	uid_t uid(0);

	if (flags & User)
	{
		errno = 0;

		passwd *_user(getpwnam(user));

		if (!_user)
			errno ? err(1, NULL) : errx(1, "Unknown user: \"%s\"", user);

		uid = _user->pw_uid;
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

	Tree tree(flags);

	if (flags & Pid)
	{
		PidMap::iterator _pid(pids.find(pid));

		if (_pid != pids.end())
		{
			Proc *proc(_pid->second);

			switch (sort)
			{
			case PidSort:
				proc->printByPid(tree);

				break;

			case NameSort:
				proc->printByName(tree);
			}
		}

		return 0;
	}

	NameMap names;

	_foreach (PidMap, pid, pids)
	{
		Proc *proc(pid->second);

		if (proc->root(uid))
			switch (sort)
			{
			case PidSort:
				proc->printByPid(tree);

				break;
			case NameSort:
				names.insert(NameMap::value_type(proc->name(), proc));
			}
	}

	switch (sort)
	{
	case NameSort:
		_foreach (NameMap, name, names)
			name->second->printByName(tree);
	default:
		return 0;
	}
}

// display a tree of processes
