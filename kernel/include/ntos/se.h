#ifndef COREOS_NTOS_SE_H
#define COREOS_NTOS_SE_H

#include "nt/ntdef.h"
#include "nt/ntstatus.h"
#include "ntos/ob.h"

typedef struct _SID {
    UCHAR Revision;
    UCHAR SubAuthorityCount;
    ULONG IdentifierAuthority;
    ULONG SubAuthority[4];
} SID;

typedef struct _TOKEN {
    OBJECT_HEADER Header;
    SID UserSid;
    UCHAR IsAdministrator;
} TOKEN;

void SeInitSystem(void);
TOKEN *SeGetSystemToken(void);
NTSTATUS SeAccessCheck(ACCESS_MASK desired, ACCESS_MASK granted);

#endif
