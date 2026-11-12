/* libbsm (OpenBSM): the audit token accessors notifyd uses. An audit_token_t is eight 32-bit words:
 * audit uid, effective uid, effective gid, real uid, real gid, pid, audit session id, pid version. */
#include <stdint.h>
typedef struct { unsigned int val[8]; } audit_token_t;

void audit_token_to_au32(audit_token_t t, uint32_t *auid, uint32_t *euid, uint32_t *egid, uint32_t *ruid,
    uint32_t *rgid, uint32_t *pid, uint32_t *asid, void *tid)
{
	if (auid) *auid = t.val[0];
	if (euid) *euid = t.val[1];
	if (egid) *egid = t.val[2];
	if (ruid) *ruid = t.val[3];
	if (rgid) *rgid = t.val[4];
	if (pid) *pid = t.val[5];
	if (asid) *asid = t.val[6];
	if (tid) { ((uint32_t *)tid)[0] = 0; ((uint32_t *)tid)[1] = 0; }
}
uint32_t audit_token_to_euid(audit_token_t t) { return t.val[1]; }
uint32_t audit_token_to_egid(audit_token_t t) { return t.val[2]; }
uint32_t audit_token_to_pid(audit_token_t t) { return t.val[5]; }
