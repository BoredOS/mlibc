#include <mlibc/resolv_conf.hpp>
#include <mlibc/allocator.hpp>
#include <stdio.h>
#include <ctype.h>

namespace mlibc {

static constexpr int MAX_NAMESERVERS = 3;

frg::vector<nameserver_data, MemoryAllocator> get_nameservers() {
	frg::vector<nameserver_data, MemoryAllocator> servers{getAllocator()};

	auto file = fopen("/etc/resolv.conf", "r");
	if (!file)
		return servers;

	char line[128];
	while (fgets(line, 128, file) && (int)servers.size() < MAX_NAMESERVERS) {
		char *pos;
		if (!strchr(line, '\n') && !feof(file)) {
			// skip truncated lines
			for (int c = getc(file); c != '\n' && c != EOF; c = getc(file));
			continue;
		}

		if (!strncmp(line, "nameserver", 10) && isspace(line[10])) {
			char *end;
			for (pos = line + 11; isspace(*pos); pos++);
			for (end = pos; *end && !isspace(*end); end++);
			*end = '\0';

			nameserver_data ns;
			ns.name = frg::string<MemoryAllocator>(pos, end - pos, getAllocator());
			servers.push(std::move(ns));
		}
	}

	fclose(file);
	return servers;
}

frg::optional<struct nameserver_data> get_nameserver() {
	auto servers = get_nameservers();
	if (servers.size() == 0)
		return frg::null_opt;
	return servers[0];
}

} // namespace mlibc
