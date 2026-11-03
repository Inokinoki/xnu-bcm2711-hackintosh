/*
 * Replaces Libinfo's mdns_module.c (DNS and mDNS through the DNSService API of mDNSResponder, which is not
 * ported). The "mdns" lookup module is simply unavailable; host names resolve through the file module only.
 */
struct si_mod_s;

struct si_mod_s *
si_module_static_mdns(void)
{
	return 0;
}
