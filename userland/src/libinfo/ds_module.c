/*
 * Replaces Libinfo's ds_module.c (the OpenDirectory lookup module, xpc pipes to opendirectoryd). The "ds" module
 * is unavailable; lookups use the file module.
 */
struct si_mod_s;

struct si_mod_s *si_module_static_ds(void) { return 0; }
void lookup_close_connections(void) { }
void _si_disable_opendirectory(void) { }
