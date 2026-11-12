/*
 * Replaces Libinfo's mdns_module.c (DNS and mDNS through the DNSService API of mDNSResponder, which is not
 * ported). The "mdns" lookup module is a small UDP stub resolver instead: the nameservers of /etc/resolv.conf are
 * asked for A and AAAA records (one query per record type, over UDP, with a few retries); no search list, no TCP
 * fallback, no mDNS, no caching. Enough for getaddrinfo / gethostbyname of a fully qualified name.
 */
#include "ils.h"
#include "netdb.h"
#include "si_module.h"

#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <dispatch/dispatch.h>

#ifndef MAXDNAME
#define MAXDNAME 1025
#endif
#define MDNS_MAX_SERVERS 3
#define MDNS_TIMEOUT_MS 2000
#define MDNS_TRIES 2

typedef struct {
	int have4, have6;
	struct in_addr a4;
	struct in6_addr a6;
	uint32_t ttl;
	char cname[MAXDNAME];
} mdns_answer_t;

/* nameservers from /etc/resolv.conf (IPv4 only) */
static int
mdns_servers(struct in_addr *out, int max)
{
	FILE *f = fopen("/etc/resolv.conf", "r");
	char line[256];
	int n = 0;

	if (f == NULL) {
		return 0;
	}
	while (n < max && fgets(line, sizeof(line), f)) {
		char *p = line;
		struct in_addr a;

		if (strncmp(p, "nameserver", 10) != 0 || (p[10] != ' ' && p[10] != '\t')) {
			continue;
		}
		p += 10;
		while (*p == ' ' || *p == '\t') {
			p++;
		}
		p[strcspn(p, " \t\r\n")] = 0;
		if (inet_pton(AF_INET, p, &a) == 1) {
			out[n++] = a;
		}
	}
	fclose(f);
	return n;
}

/* length of the (possibly compressed) name at p */
static int
mdns_skip_name(const uint8_t *msg, int len, int p)
{
	while (p < len) {
		int l = msg[p];

		if (l == 0) {
			return p + 1;
		}
		if ((l & 0xc0) == 0xc0) {
			return p + 2;
		}
		p += l + 1;
	}
	return -1;
}

/* expand the name at p into dst; returns false on a malformed name */
static int
mdns_expand_name(const uint8_t *msg, int len, int p, char *dst, size_t dstlen)
{
	size_t o = 0;
	int hops = 0;

	while (p < len) {
		int l = msg[p];

		if (l == 0) {
			if (o > 0) {
				o--;    /* trailing dot */
			}
			dst[o] = 0;
			return 1;
		}
		if ((l & 0xc0) == 0xc0) {
			if (p + 1 >= len || ++hops > 16) {
				return 0;
			}
			p = ((l & 0x3f) << 8) | msg[p + 1];
			continue;
		}
		if (p + 1 + l > len || o + l + 2 > dstlen) {
			return 0;
		}
		memcpy(dst + o, msg + p + 1, l);
		o += l;
		dst[o++] = '.';
		p += l + 1;
	}
	return 0;
}

static int
mdns_encode_name(const char *name, uint8_t *out, size_t outlen)
{
	size_t o = 0;

	while (*name) {
		const char *dot = strchr(name, '.');
		size_t l = dot ? (size_t)(dot - name) : strlen(name);

		if (l == 0 && dot == NULL) {
			break;
		}
		if (l == 0 || l > 63 || o + l + 2 > outlen) {
			return -1;
		}
		out[o++] = (uint8_t)l;
		memcpy(out + o, name, l);
		o += l;
		name += l + (dot ? 1 : 0);
	}
	if (o + 1 > outlen) {
		return -1;
	}
	out[o++] = 0;
	return (int)o;
}

/* one query/answer exchange for type (ns_t_a or ns_t_aaaa); returns 0 and fills ans on success */
static int
mdns_query(const char *name, int type, mdns_answer_t *ans)
{
	struct in_addr servers[MDNS_MAX_SERVERS];
	int ns = mdns_servers(servers, MDNS_MAX_SERVERS);
	uint8_t q[512], r[1500];
	uint16_t id = (uint16_t)(time(NULL) * 2654435761u + getpid());
	int qlen;

	if (ns == 0) {
		return -1;
	}
	memset(q, 0, 12);
	q[0] = id >> 8;
	q[1] = id & 0xff;
	q[2] = 0x01;    /* RD */
	q[5] = 1;       /* QDCOUNT */
	qlen = mdns_encode_name(name, q + 12, sizeof(q) - 16);
	if (qlen < 0) {
		return -1;
	}
	qlen += 12;
	q[qlen++] = 0;
	q[qlen++] = (uint8_t)type;
	q[qlen++] = 0;
	q[qlen++] = 1;  /* IN */

	for (int t = 0; t < MDNS_TRIES; t++) {
		for (int s = 0; s < ns; s++) {
			struct sockaddr_in sa = { .sin_len = sizeof(sa), .sin_family = AF_INET, .sin_port = htons(53), .sin_addr = servers[s] };
			struct pollfd pfd;
			int fd = socket(AF_INET, SOCK_DGRAM, 0);
			ssize_t n;

			if (fd < 0) {
				return -1;
			}
			if (sendto(fd, q, qlen, 0, (struct sockaddr *)&sa, sizeof(sa)) != qlen) {
				close(fd);
				continue;
			}
			pfd.fd = fd;
			pfd.events = POLLIN;
			if (poll(&pfd, 1, MDNS_TIMEOUT_MS) <= 0) {
				close(fd);
				continue;
			}
			n = recv(fd, r, sizeof(r), 0);
			close(fd);
			if (n < 12 || r[0] != q[0] || r[1] != q[1] || !(r[2] & 0x80)) {
				continue;
			}
			if ((r[3] & 0x0f) != 0) {
				return -1;    /* NXDOMAIN, SERVFAIL, ... */
			}
			{
				int qd = (r[4] << 8) | r[5], an = (r[6] << 8) | r[7], p = 12;

				for (int i = 0; i < qd; i++) {
					p = mdns_skip_name(r, (int)n, p);
					if (p < 0) {
						return -1;
					}
					p += 4;
				}
				for (int i = 0; i < an && p >= 0 && p + 10 <= n; i++) {
					int rt, rdl;

					p = mdns_skip_name(r, (int)n, p);
					if (p < 0 || p + 10 > n) {
						return -1;
					}
					rt = (r[p] << 8) | r[p + 1];
					rdl = (r[p + 8] << 8) | r[p + 9];
					if (p + 10 + rdl > n) {
						return -1;
					}
					if (rt == ns_t_a && rdl == 4 && !ans->have4) {
						memcpy(&ans->a4, r + p + 10, 4);
						ans->have4 = 1;
						ans->ttl = ((uint32_t)r[p + 4] << 24) | (r[p + 5] << 16) | (r[p + 6] << 8) | r[p + 7];
					} else if (rt == ns_t_aaaa && rdl == 16 && !ans->have6) {
						memcpy(&ans->a6, r + p + 10, 16);
						ans->have6 = 1;
						ans->ttl = ((uint32_t)r[p + 4] << 24) | (r[p + 5] << 16) | (r[p + 6] << 8) | r[p + 7];
					} else if (rt == ns_t_cname && ans->cname[0] == 0) {
						mdns_expand_name(r, (int)n, p + 10, ans->cname, sizeof(ans->cname));
					}
					p += 10 + rdl;
				}
			}
			return (type == ns_t_a ? ans->have4 : ans->have6) ? 0 : -1;
		}
	}
	return -1;
}

static void
mdns_close(si_mod_t *si)
{
}

static int
mdns_is_valid(si_mod_t *si, si_item_t *item)
{
	return 0;
}

static si_item_t *
mdns_hostbyname(si_mod_t *si, const char *name, int af, const char *interface, uint32_t *err)
{
	mdns_answer_t ans;
	char *addrs[2] = { NULL, NULL };
	char *aliases[1] = { NULL };
	si_item_t *out = NULL;

	if (err != NULL) {
		*err = SI_STATUS_NO_ERROR;
	}
	if (name == NULL || si == NULL || (af != AF_INET && af != AF_INET6)) {
		if (err != NULL) {
			*err = SI_STATUS_H_ERRNO_NO_RECOVERY;
		}
		return NULL;
	}
	memset(&ans, 0, sizeof(ans));
	if (mdns_query(name, af == AF_INET ? ns_t_a : ns_t_aaaa, &ans) != 0) {
		if (err != NULL) {
			*err = SI_STATUS_H_ERRNO_HOST_NOT_FOUND;
		}
		return NULL;
	}
	addrs[0] = af == AF_INET ? (char *)&ans.a4 : (char *)&ans.a6;
	if (af == AF_INET) {
		out = (si_item_t *)LI_ils_create("L4488s*44a", (unsigned long)si, CATEGORY_HOST_IPV4, 1, (uint64_t)time(NULL) + ans.ttl, 0LL,
		    name, aliases, af, 4, addrs);
	} else {
		out = (si_item_t *)LI_ils_create("L4488s*44c", (unsigned long)si, CATEGORY_HOST_IPV6, 1, (uint64_t)time(NULL) + ans.ttl, 0LL,
		    name, aliases, af, 16, addrs);
	}
	if (out == NULL && err != NULL) {
		*err = SI_STATUS_H_ERRNO_NO_RECOVERY;
	}
	return out;
}

static si_item_t *
mdns_hostbyaddr(si_mod_t *si, const void *addr, int af, const char *interface, uint32_t *err)
{
	if (err != NULL) {
		*err = SI_STATUS_H_ERRNO_HOST_NOT_FOUND;    /* no PTR lookups */
	}
	return NULL;
}

static si_list_t *
mdns_addrinfo(si_mod_t *si, const void *node, const void *serv, uint32_t family, uint32_t socktype, uint32_t proto, uint32_t flags, const char *interface, uint32_t *err)
{
	mdns_answer_t ans;
	uint16_t port;
	int wantv4 = 1, wantv6 = 1;

	if (si == NULL) {
		if (err != NULL) {
			*err = SI_STATUS_H_ERRNO_NO_RECOVERY;
		}
		return NULL;
	}
	if (family == AF_INET6) {
		if ((flags & AI_V4MAPPED) == 0) {
			wantv4 = 0;
		}
	} else if (family == AF_INET) {
		wantv6 = 0;
	} else if (family != AF_UNSPEC) {
		return NULL;
	}
	if (err != NULL) {
		*err = SI_STATUS_NO_ERROR;
	}

	if ((flags & AI_NUMERICSERV) != 0) {
		port = (serv == NULL) ? 0 : *(const uint16_t *)serv;
	} else if (_gai_serv_to_port(serv, proto, &port) != 0) {
		if (err != NULL) {
			*err = SI_STATUS_EAI_NONAME;
		}
		return NULL;
	}

	if (node == NULL) {
		return NULL;
	}
	if ((flags & AI_NUMERICHOST) != 0) {
		struct in_addr a4;
		struct in6_addr a6;

		if (family == AF_INET) {
			memcpy(&a4, node, sizeof(a4));
			return si_addrinfo_list(si, flags, socktype, proto, &a4, NULL, port, 0, NULL, NULL);
		}
		if (family == AF_INET6) {
			memcpy(&a6, node, sizeof(a6));
			return si_addrinfo_list(si, flags, socktype, proto, NULL, &a6, port, 0, NULL, NULL);
		}
		return NULL;
	}

	memset(&ans, 0, sizeof(ans));
	if (wantv4) {
		mdns_query(node, ns_t_a, &ans);
	}
	if (wantv6) {
		mdns_query(node, ns_t_aaaa, &ans);
	}
	if (!ans.have4 && !ans.have6) {
		if (err != NULL) {
			*err = SI_STATUS_EAI_NONAME;
		}
		return NULL;
	}
	return si_addrinfo_list(si, flags, socktype, proto, ans.have4 ? &ans.a4 : NULL, ans.have6 ? &ans.a6 : NULL, port, 0,
	           ans.cname[0] ? ans.cname : NULL, ans.cname[0] ? ans.cname : NULL);
}

si_mod_t *
si_module_static_mdns(void)
{
	static const struct si_mod_vtable_s mdns_vtable = {
		.sim_close = &mdns_close,
		.sim_is_valid = &mdns_is_valid,
		.sim_host_byname = &mdns_hostbyname,
		.sim_host_byaddr = &mdns_hostbyaddr,
		.sim_addrinfo = &mdns_addrinfo,
	};
	static si_mod_t si = {
		.vers = 1,
		.refcount = 1,
		.flags = SI_MOD_FLAG_STATIC,
		.private = NULL,
		.vtable = &mdns_vtable,
	};
	static dispatch_once_t once;

	dispatch_once(&once, ^{
		si.name = strdup("mdns");
	});
	return (si_mod_t *)&si;
}
