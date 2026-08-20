#include "loader_policy.h"

int musl_bsd_loader_is_secure(unsigned long at_secure,
                              uid_t real_uid,
                              uid_t effective_uid,
                              gid_t real_gid,
                              gid_t effective_gid) {
    return at_secure != 0 || real_uid != effective_uid || real_gid != effective_gid;
}
