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
#include <sstream>
#include <string>
#include <vector>

#ifndef __GLIBC__
#include <libgen.h>
#endif

#ifdef HAVE_TERMCAP_H
#include <termcap.h>
#elif defined(HAVE_NCURSES_TERMCAP_H)
#include <ncurses/termcap.h>
#elif defined(HAVE_NCURSES_TERM_H)
#include <ncurses/ncurses.h>
#include <ncurses/term.h>
#elif defined(HAVE_TERM_H)
#include <curses.h>
#include <term.h>
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
#include <sys/utsname.h>
#include <unistd.h>
#include <vis.h>

#include "foreach.hpp"

namespace kvm
{

#if HAVE_DECL_KERN_PROC_PROC
const int All(KERN_PROC_PROC);
#elif HAVE_DECL_KERN_PROC_KTHREAD
const int All(KERN_PROC_KTHREAD);
#else
const int All(KERN_PROC_ALL);
#endif

template <typename Type>
inline Type *getprocs(kvm_t *kd, int &count);

template <typename Type>
inline char **getargv(kvm_t *kd, const Type *proc);

template <typename Type>
inline pid_t pid(Type *proc);

template <typename Type>
inline pid_t ppid(Type *proc);

template <typename Type>
inline uid_t ruid(Type *proc);

template <typename Type>
inline char *comm(Type *proc);

#ifndef HAVE_STRUCT_KINFO_PROC2
typedef kinfo_proc Proc;

const int Flags(O_RDONLY);

template <>
inline kinfo_proc *getprocs(kvm_t *kd, int &count)
{
	return kvm_getprocs(kd, All, 0, &count);
}

template <>
inline char **getargv(kvm_t *kd, const kinfo_proc *proc)
{
	return kvm_getargv(kd, proc, 0);
}
#else
typedef kinfo_proc2 Proc;

const int Flags(KVM_NO_FILES);

template <>
inline kinfo_proc2 *getprocs(kvm_t *kd, int &count)
{
	return kvm_getproc2(kd, All, 0, sizeof (kinfo_proc2), &count);
}

template <>
inline char **getargv(kvm_t *kd, const kinfo_proc2 *proc)
{
	return kvm_getargv2(kd, proc, 0);
}
#endif

template <>
inline pid_t pid(Proc *proc)
{
#	ifdef HAVE_STRUCT_KINFO_PROCX_KI_PID
	return proc->ki_pid;
#	elif defined(HAVE_STRUCT_KINFO_PROCX_KP_PID)
	return proc->kp_pid;
#	elif defined(HAVE_STRUCT_KINFO_PROCX_P_PID)
	return proc->p_pid;
#	endif
}

template <>
inline pid_t ppid(Proc *proc)
{
#	ifdef HAVE_STRUCT_KINFO_PROCX_KI_PPID
	return proc->ki_ppid;
#	elif defined(HAVE_STRUCT_KINFO_PROCX_KP_PPID)
	return proc->kp_ppid;
#	elif defined(HAVE_STRUCT_KINFO_PROCX_P_PPID)
	return proc->p_ppid;
#	endif
}

template <>
inline uid_t ruid(Proc *proc)
{
#	ifdef HAVE_STRUCT_KINFO_PROCX_KI_RUID
	return proc->ki_ruid;
#	elif defined(HAVE_STRUCT_KINFO_PROCX_KP_RUID)
	return proc->kp_ruid;
#	elif defined(HAVE_STRUCT_KINFO_PROCX_P_RUID)
	return proc->p_ruid;
#	endif
}

template <>
inline char *comm(Proc *proc)
{
#	ifdef HAVE_STRUCT_KINFO_PROCX_KI_COMM
	return proc->ki_comm;
#	elif defined(HAVE_STRUCT_KINFO_PROCX_KP_COMM)
	return proc->kp_comm;
#	elif defined(HAVE_STRUCT_KINFO_PROCX_P_COMM)
	return proc->p_comm;
#	endif
}

}

enum Flags
{
	Arguments	= 0x0001,
	Ascii		= 0x0002,
	NoCompact	= 0x0004,
	Highlight	= 0x0008,
	Vt100		= 0x0010,
	ShowKernel	= 0x0020,
	Long		= 0x0040,
	NumericSort	= 0x0080,
	ShowPids	= 0x0100,
	ShowTitles	= 0x0200,
	UidChanges	= 0x0400,
	Unicode		= 0x0800,
	Pid			= 0x1000,
	User		= 0x2000
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
	size_t duplicate_;

public:
	Tree(const uint16_t &flags) : flags_(flags), vt100_(false), maxWidth_(0), width_(0), max_(false), suppress_(false), duplicate_(0)
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

			wchar_t wides[] = { horizontal_, vertical_, upAndRight_, verticalAndRight_, downAndHorizontal_ };
			char *buffer = new char[MB_CUR_MAX];

			for (int index(0); index != sizeof (wides) / sizeof (*wides); ++index)
			{
				int size;

				if ((size = std::wctomb(buffer, wides[index])) == -1)
				{
					delete [] buffer;
					goto vt100;
				}

				wchar_t wide;

				if (std::mbtowc(&wide, buffer, size) == -1)
				{
					delete [] buffer;
					goto vt100;
				}

				if (wide != wides[index])
				{
					delete [] buffer;
					goto vt100;
				}
			}

			delete [] buffer;
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
#			if !defined(HAVE_TERMCAP_H) && !defined(HAVE_NCURSES_TERMCAP_H)
			int code;

			if (setupterm(NULL, 1, &code) == OK)
			{
				maxWidth_ = tigetnum(const_cast<char *>("cols"));

				if (tigetflag(const_cast<char *>("am")) && !tigetflag(const_cast<char *>("xenl")))
					suppress_ = true;
			}
#			else
			char buffer[1024], *term(std::getenv("TERM"));

			if (term != NULL && tgetent(buffer, term) == 1)
			{
				maxWidth_ = tgetnum("co");

				if (tgetflag("am") && !tgetflag("xn"))
					suppress_ = true;
			}
#			endif
			else
				maxWidth_ = 80;
		}
	}

	void print(const std::string &string, bool highlight, size_t duplicate)
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

		size_t size(0);

		if (duplicate)
		{
			std::ostringstream string;

			string << duplicate << "*[";

			size = string.str().size();

			print(size, None, "%s", string.str().c_str());

			++duplicate_;
		}

		print(string.size(), highlight ? Bright : None, "%s", string.c_str());

		branches_.push_back(Branch(!(flags_ & Arguments) ? size + string.size() + 1 : 2));
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
		if (duplicate_)
		{
			print(duplicate_, None, "%s", std::string(duplicate_, ']').c_str());

			duplicate_ = 0;
		}

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

				wide += L'+';

				size_t size(std::wcstombs(NULL, wide.c_str(), 0) + 1);

				string = static_cast<char *>(std::malloc(size));

				std::wcstombs(string, wide.c_str(), size);

				if (previous)
				{
					segments_.back().string_ = string;

					return;
				}
			}

		segments_.push_back(Segment(width, escape, string));
	}
};

template <typename Type>
struct Proc
{
	typedef std::multimap<pid_t, Proc<Type> *> PidMap;
	typedef std::multimap<std::string, Proc<Type> *> NameMap;

private:
	const uint16_t &flags_;
	kvm_t *kd_;
	Type *proc_;
	mutable std::string name_, print_;
	Proc<Type> *parent_;
	PidMap childrenByPid_;
	NameMap childrenByName_;
	bool highlight_, root_;
	int8_t compact_;
	size_t duplicate_;

public:
	inline Proc(const uint16_t &flags, kvm_t *kd, Type *proc) : flags_(flags), kd_(kd), proc_(proc), parent_(NULL), highlight_(false), root_(false), compact_(-1), duplicate_(0) {}

	inline const std::string &name() const
	{
		if (name_.empty())
			name_ = visual(kvm::comm(proc_));

		return name_;
	}

	inline pid_t parent() const { return kvm::ppid(proc_); }
	inline pid_t pid() const { return kvm::pid(proc_); }

	inline void child(Proc *proc)
	{
		if (proc == this)
			return;

		proc->parent_ = this;

		childrenByPid_.insert(typename PidMap::value_type(proc->pid(), proc));
		childrenByName_.insert(typename NameMap::value_type(proc->name(), proc));
	}

	inline void highlight()
	{
		highlight_ = true;

		if (parent_)
			parent_->highlight();
	}

	inline bool compact()
	{
		if (compact_ == -1)
			compact_ = compact(childrenByName_);

		return compact_;
	}

	bool root(uid_t uid)
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

	inline void printByPid(Tree &tree) const
	{
		print(tree, childrenByPid_);
	}

	inline void printByName(Tree &tree) const
	{
		print(tree, childrenByName_);
	}

	static bool compact(NameMap &names)
	{
		Proc *previous(NULL);
		bool compact(true);

		_tforeach (NameMap, name, names)
		{
			Proc *proc(name->second);

			if (proc->duplicate_)
				continue;

			size_t duplicate(proc->compact());

			if (compact && duplicate && (!previous || proc->print() == previous->print()))
				previous = proc;
			else
				compact = false;

			size_t count(names.count(name->first));

			if (!duplicate || count == 1)
				continue;

			_forall(typename NameMap::iterator, n4me, (++name)--, names.upper_bound(name->first))
			{
				Proc *pr0c(n4me->second);

				if (pr0c->compact() && Proc::compact(proc, pr0c))
					duplicate += ++pr0c->duplicate_;
			}

			if (duplicate != 1)
				proc->duplicate_ = duplicate;
		}

		return compact;
	}

private:
	inline std::string visual(const char *string) const
	{
		std::string visual(std::strlen(string) * 4 + 1, '\0');

		visual.resize(strvis(const_cast<char *>(visual.data()), string, VIS_TAB | VIS_NL | VIS_NOSLASH));

		return visual;
	}

	template <typename Map>
	void print(Tree &tree, const Map &children) const
	{
		if (duplicate_ == 1)
			return;

		print(tree);

		size_t size(children.size()), last(size - 1);

		_tforeach (const Map, child, children)
		{
			Proc<Type> *proc(child->second);
			bool l4st(_index + (proc->duplicate_ ? proc->duplicate_ - 1 : 0) == last);

			if (!l4st)
			{
				l4st = true;

				for (++child; child != _end; ++child)
					if (child->second->duplicate_ != 1)
					{
						l4st = false;

						break;
					}

				--child;
			}

			proc->print(tree(!_index, l4st), proc->template children<Map>());

			if (l4st)
				break;
		}

		tree.pop(size);
	}

	void print(Tree &tree) const
	{
		tree.print(print(), highlight_, duplicate_);

		if (flags_ & Arguments)
		{
			char **argv(kvm::getargv(kd_, proc_));

			if (argv && *argv)
				for (++argv; *argv; ++argv)
					tree.printArg(visual(*argv), !*(argv + 1));

			tree.done();
		}
	}

	const std::string &print() const
	{
		if (print_.empty())
		{
			std::ostringstream print;

			if (flags_ & ShowTitles)
			{
				char **argv(kvm::getargv(kd_, proc_));

				if (argv)
					print << visual(*argv);
				else
					print << name();
			}
			else
				print << name();

			bool p1d(flags_ & ShowPids), args(flags_ & Arguments);
			bool change(flags_ & UidChanges && (root_ ? !(flags_ & User) && uid() : parent_ && uid() != parent_->uid()));
			bool parens((p1d || change) && !args);

			if (parens)
				print << '(';

			if (p1d)
			{
				if (!parens)
					print << ',';

				print << pid();
			}

			if (change)
			{
				if (!parens || p1d)
					print << ',';

				passwd *user(getpwuid(uid()));

				print << user->pw_name;
			}

			if (parens)
				print << ')';

			print_ = print.str();
		}

		return print_;
	}

	inline uid_t uid() const { return kvm::ruid(proc_); }

	template <typename Map>
	inline const Map &children() const;

	inline bool hasChildren() const { return childrenByName_.size(); }
	inline Proc<Type> *child() const { return childrenByName_.begin()->second; }

	inline static bool compact(Proc<Type> *one, Proc<Type> *two)
	{
		if (one->print() != two->print())
			return false;

		if (one->hasChildren() != two->hasChildren())
			return false;

		if (one->hasChildren() && !compact(one->child(), two->child()))
			return false;

		if (two->highlight_)
			one->highlight_ = true;

		return true;
	}
};

template <> template <>
inline const Proc<kvm::Proc>::PidMap &Proc<kvm::Proc>::children() const
{
	return childrenByPid_;
}

template <> template <>
inline const Proc<kvm::Proc>::NameMap &Proc<kvm::Proc>::children() const
{
	return childrenByName_;
}

static void help(char *program, option options[], int code = 0)
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
		case 'c':
			if (name != "no-compact")
				continue;
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

template <typename Type, long minimum, long maximum>
static Type value(char *program, option options[], bool *success = NULL)
{
	char *end;
	long value(std::strtol(optarg, &end, 0));

	errno = 0;

	if (optarg == end || *end != '\0')
		if (success)
			*success = false;
		else
		{
			warnx("Number is invalid: \"%s\"", optarg);
			help(program, options, 1);
		}
	else if (value < minimum || value == LONG_MIN && errno == ERANGE)
	{
		warnx("Number is too small: \"%s\"", optarg);
		help(program, options, 1);
	}
	else if (value > maximum || value == LONG_MAX && errno == ERANGE)
	{
		warnx("Number is too large: \"%s\"", optarg);
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
		{ "no-compact", no_argument, NULL, 'c' },
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
		{ "version", optional_argument, NULL, 'V' },
		{ "pid", required_argument, NULL, 0 },
		{ "user", required_argument, NULL, 0 },
		{ NULL, 0, NULL, 0 }
	};
	int option, index;
	uint16_t flags(0);
	char *program(argv[0]);

	while ((option = getopt_long(argc, argv, "aAchH::GklnptuUV::", options, &index)) != -1)
		switch (option)
		{
		case 'a':
			flags |= Arguments | NoCompact; break;
		case 'A':
			flags |= Ascii;
			flags &= ~Vt100;
			flags &= ~Unicode;

			break;
		case 'c':
			flags |= NoCompact; break;
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
			flags |= NoCompact | ShowPids; break;
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
			{
				std::string version(optarg ? optarg : "");

				if (version == "s" || version == "short")
					std::printf(PACKAGE_TARNAME " " PACKAGE_VERSION "\n");
				else
				{
					utsname name;

					if (uname(&name))
						err(1, NULL);

					std::printf(PACKAGE_TARNAME " " PACKAGE_VERSION " - %s %s %s\n", name.sysname, name.release, name.machine);
				}

				if (version == "l" || version == "license")
					std::printf("\n"
						"   Copyright 2010 Douglas Thrift\n\n"
						"   Licensed under the Apache License, Version 2.0 (the \"License\");\n"
						"   you may not use this file except in compliance with the License.\n"
						"   You may obtain a copy of the License at\n\n"
						"       http://www.apache.org/licenses/LICENSE-2.0\n\n"
						"   Unless required by applicable law or agreed to in writing, software\n"
						"   distributed under the License is distributed on an \"AS IS\" BASIS,\n"
						"   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
						"   See the License for the specific language governing permissions and\n"
						"   limitations under the License.\n");

				std::exit(0);
			}
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
		bool success(false);

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

template <typename Type, int Flags>
static void tree(pid_t hpid, pid_t pid, uint16_t flags, uid_t uid)
{
	char error[_POSIX2_LINE_MAX];
	kvm_t *kd(kvm_openfiles(NULL, _PATH_DEVNULL, NULL, Flags, error));

	if (!kd)
		errx(1, "%s", error);

	int count;
	Type *procs(kvm::getprocs<Type>(kd, count));

	if (!procs)
		errx(1, "%s", kvm_geterr(kd));

	typedef Type *Pointer;
	typename Proc<Type>::PidMap pids;

	_forall (Pointer, proc, procs, procs + count)
		if (flags & ShowKernel || kvm::ppid(proc) > 0 || kvm::pid(proc) == 1)
			pids.insert(typename Proc<Type>::PidMap::value_type(kvm::pid(proc), new Proc<Type>(flags, kd, proc)));

	enum { PidSort, NameSort } sort(flags & NumericSort ? PidSort : NameSort);

	_tforeach (typename Proc<Type>::PidMap, pid, pids)
	{
		Proc<Type> *proc(pid->second);

		if (proc->parent() == -1)
			continue;

		typename Proc<Type>::PidMap::iterator parent(pids.find(proc->parent()));

		if (parent != pids.end())
			parent->second->child(proc);
	}

	if (flags & Highlight)
	{
		typename Proc<Type>::PidMap::iterator pid(pids.find(hpid));

		if (pid != pids.end())
			pid->second->highlight();
	}

	Tree tree(flags);

	if (flags & Pid)
	{
		typename Proc<Type>::PidMap::iterator p1d(pids.find(pid));

		if (p1d != pids.end())
		{
			Proc<Type> *proc(p1d->second);

			if (!(flags & NoCompact))
				proc->compact();

			switch (sort)
			{
			case PidSort:
				proc->printByPid(tree);

				break;
			case NameSort:
				proc->printByName(tree);
			}
		}
	}
	else
	{
		typename Proc<Type>::NameMap names;

		_tforeach (typename Proc<Type>::PidMap, pid, pids)
		{
			Proc<Type> *proc(pid->second);

			if (proc->root(uid))
				names.insert(typename Proc<Type>::NameMap::value_type(proc->name(), proc));
		}

		if (!(flags & NoCompact))
			Proc<Type>::compact(names);

		switch (sort)
		{
		case PidSort:
			_tforeach (typename Proc<Type>::PidMap, pid, pids)
			{
				Proc<Type> *proc(pid->second);

				if (proc->root(uid))
					proc->printByPid(tree);
			}

			break;
		case NameSort:
			_tforeach (typename Proc<Type>::NameMap, name, names)
				name->second->printByName(tree);
		}
	}

	_tforeach (typename Proc<Type>::PidMap, pid, pids)
		delete pid->second;
}

int main(int argc, char *argv[])
{
	pid_t hpid(0), pid(0);
	char *user(NULL);
	uint16_t flags(options(argc, argv, hpid, pid, user));
	uid_t uid(0);

	if (flags & User)
	{
		errno = 0;

		passwd *us3r(getpwnam(user));

		if (!us3r)
			errno ? err(1, NULL) : errx(1, "Unknown user: \"%s\"", user);

		uid = us3r->pw_uid;
	}

	tree<kvm::Proc, kvm::Flags>(hpid, pid, flags, uid);

	return 0;
}

// display a tree of processes
