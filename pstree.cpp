// DT PS Tree
//
// Douglas Thrift
//
// $Id$

#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <paths.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>

int main(int argc, char *argv[])
{
	char error[_POSIX2_LINE_MAX];
	kvm_t *kd = kvm_openfiles(NULL, _PATH_DEVNULL, NULL, O_RDONLY, error);

	if (!kd)
		errx(1, "%s", error);

	int count;
	struct kinfo_proc *procs = kvm_getprocs(kd, KERN_PROC_PROC, 0, &count);

	if (!procs)
		errx(1, "%s", kvm_geterr(kd));

	for (struct kinfo_proc *proc = procs, *end = procs + count; proc != end; ++proc)
		printf("%s %i %i\n", proc->ki_comm, proc->ki_pid, proc->ki_ppid);

	return 0;
}
