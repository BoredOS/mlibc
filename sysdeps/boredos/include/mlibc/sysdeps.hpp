#pragma once

#include <mlibc/sysdep-signatures.hpp>

namespace mlibc {

struct BoredOSSysdepTags :
	LibcPanic,
	LibcLog,
	Isatty,
	Write,
	Read,
	Open,
	Close,
	Seek,
	Exit,
	Sleep,
	AnonAllocate,
	AnonFree,
	VmMap,
	VmUnmap,
	ClockGet,
	FutexWait,
	FutexWake,
	Fork,
	Execve,
	Waitpid,
	GetPid,
	GetCwd,
	Chdir,
	Mkdir,
	Unlinkat,
	Dup,
	Dup2,
	Pipe,
	Fcntl,
	Poll,
	Ioctl,
	Socket,
	Connect,
	Bind,
	Listen,
	Accept,
	Sendto,
	Recvfrom,
	Sigaction,
	Sigprocmask,
	Sigpending,
	Kill,
	Access,
	Stat,
	Tcgetattr,
	Tcsetattr,
	TcbSet
{};

template<typename Tag>
using Sysdeps = SysdepOf<BoredOSSysdepTags, Tag>;

struct SysdepTraits {
	static constexpr bool usesRtNetlink = false;
};

} // namespace mlibc
