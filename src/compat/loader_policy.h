#ifndef MUSL_BSD_LOADER_POLICY_H
#define MUSL_BSD_LOADER_POLICY_H

#include <sys/types.h>

int musl_bsd_loader_is_secure(unsigned long at_secure,
                              uid_t real_uid,
                              uid_t effective_uid,
                              gid_t real_gid,
                              gid_t effective_gid);

#endif
