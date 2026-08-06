#ifndef _MLIBC_RESOLV_CONF
#define _MLIBC_RESOLV_CONF

#include <frg/string.hpp>
#include <frg/optional.hpp>
#include <frg/vector.hpp>
#include <mlibc/allocator.hpp>

namespace mlibc {

struct nameserver_data {
	nameserver_data()
	: name(getAllocator()) {}
	frg::string<MemoryAllocator> name;
	// for in the future we can also store options here
};

// Returns all nameservers from /etc/resolv.conf (up to 3), in order.
frg::vector<nameserver_data, MemoryAllocator> get_nameservers();

// Legacy single-server accessor — returns the first nameserver, or null_opt.
frg::optional<struct nameserver_data> get_nameserver();

} // namespace mlibc

#endif // _MLIBC_RESOLV_CONF
