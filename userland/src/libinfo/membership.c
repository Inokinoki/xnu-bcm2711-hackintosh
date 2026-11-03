/*
 * Replaces Libinfo's membership.c, which talks to OpenDirectory (closed) over xpc pipes. Without a directory
 * service every identity-to-UUID translation and membership query fails with ENOENT; users and groups still
 * resolve through the file module (/etc/passwd, /etc/group).
 */
#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

typedef unsigned char uuid_t[16];
typedef struct nt_sid nt_sid_t;

int mbr_uid_to_uuid(uid_t uid, uuid_t uu) { (void)uid; (void)uu; return ENOENT; }
int mbr_gid_to_uuid(gid_t gid, uuid_t uu) { (void)gid; (void)uu; return ENOENT; }
int mbr_sid_to_uuid(const nt_sid_t *sid, uuid_t uu) { (void)sid; (void)uu; return ENOENT; }
int mbr_uuid_to_id(const uuid_t uu, id_t *uid_or_gid, int *id_type) { (void)uu; (void)uid_or_gid; (void)id_type; return ENOENT; }
int mbr_uuid_to_sid(const uuid_t uu, nt_sid_t *sid) { (void)uu; (void)sid; return ENOENT; }
int mbr_sid_to_string(const nt_sid_t *sid, char *string) { (void)sid; (void)string; return EINVAL; }
int mbr_string_to_sid(const char *string, nt_sid_t *sid) { (void)string; (void)sid; return EINVAL; }
int mbr_check_membership(const uuid_t user, const uuid_t group, int *ismember) { (void)user; (void)group; if (ismember) *ismember = 0; return ENOENT; }
int mbr_reset_cache(void) { return 0; }
int mbr_user_name_to_uuid(const char *name, uuid_t uu) { (void)name; (void)uu; return ENOENT; }
int mbr_group_name_to_uuid(const char *name, uuid_t uu) { (void)name; (void)uu; return ENOENT; }
int mbr_check_membership_by_id(uuid_t user, gid_t group, int *ismember) { (void)user; (void)group; if (ismember) *ismember = 0; return ENOENT; }
int mbr_check_membership_refresh(const uuid_t user, uuid_t group, int *ismember) { (void)user; (void)group; if (ismember) *ismember = 0; return ENOENT; }
int mbr_uuid_to_string(const uuid_t uu, char *string) { (void)uu; (void)string; return EINVAL; }
int mbr_string_to_uuid(const char *string, uuid_t uu) { (void)string; (void)uu; return EINVAL; }
int mbr_uuid_to_sid_type(const uuid_t uu, nt_sid_t *sid, int *id_type) { (void)uu; (void)sid; (void)id_type; return ENOENT; }
int mbr_set_identifier_ttl(int id_type, const void *identifier, size_t identifier_size, unsigned int seconds) { (void)id_type; (void)identifier; (void)identifier_size; (void)seconds; return ENOENT; }
int mbr_identifier_translate(int id_type, const void *identifier, size_t identifier_size, int target_type, void **result, int *rec_type) { (void)id_type; (void)identifier; (void)identifier_size; (void)target_type; (void)result; (void)rec_type; return ENOENT; }
int mbr_check_membership_ext(int userid_type, const void *userid, size_t userid_size, int groupid_type, const void *groupid, int refresh, int *isMember) { (void)userid_type; (void)userid; (void)userid_size; (void)groupid_type; (void)groupid; (void)refresh; if (isMember) *isMember = 0; return ENOENT; }
int mbr_close_connections(void) { return 0; }
