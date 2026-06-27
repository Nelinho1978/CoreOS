#include "ntos/se.h"
#include "coreos/printk.h"

static TOKEN g_system_token;

void SeInitSystem(void) {
    g_system_token.Header.Type = ObTypeToken;
    g_system_token.Header.RefCount = 1;
    g_system_token.Header.Name[0] = 'S';
    g_system_token.Header.Name[1] = 'Y';
    g_system_token.Header.Name[2] = 'S';
    g_system_token.Header.Name[3] = 'T';
    g_system_token.Header.Name[4] = 'E';
    g_system_token.Header.Name[5] = 'M';
    g_system_token.Header.Name[6] = '\0';
    g_system_token.UserSid.Revision = 1;
    g_system_token.UserSid.SubAuthorityCount = 1;
    g_system_token.UserSid.IdentifierAuthority = 5;
    g_system_token.UserSid.SubAuthority[0] = 18;
    g_system_token.IsAdministrator = 1;

    kputs("[Se] Security Reference Monitor inicializado\n");
}

TOKEN *SeGetSystemToken(void) {
    return &g_system_token;
}

NTSTATUS SeAccessCheck(ACCESS_MASK desired, ACCESS_MASK granted) {
    if ((granted & desired) == desired) {
        return STATUS_SUCCESS;
    }
    return STATUS_ACCESS_DENIED;
}
