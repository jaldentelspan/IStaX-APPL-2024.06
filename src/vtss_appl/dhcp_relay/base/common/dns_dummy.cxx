#include "dhcpd.h"

ns_updrec *minires_mkupdrec (int, const char *, unsigned int,
                 unsigned int, unsigned long)
{
    printf("%s dummy function not yet implemented\n", __FUNCTION__);
    return NULL;
}

isc_result_t minires_nupdate (res_state, ns_updrec *)
{
    printf("%s dummy function not yet implemented\n", __FUNCTION__);
    return (isc_result_t)0;
}

void minires_freeupdrec (ns_updrec *)
{
    printf("%s dummy function not yet implemented\n", __FUNCTION__);
}

int minires_ninit (res_state)
{
    printf("%s dummy function not yet implemented\n", __FUNCTION__);
    return 0;
}

int MRns_name_compress(const char *, u_char *, size_t, const unsigned char **,
               const unsigned char **)
{
    printf("%s dummy function not yet implemented\n", __FUNCTION__);
    return 0;
}

int MRns_name_unpack(const unsigned char *, const unsigned char *,
             const unsigned char *, unsigned char *, size_t)
{
    printf("%s dummy function not yet implemented\n", __FUNCTION__);
    return 0;
}

int MRns_name_ntop(const unsigned char *, char *, size_t)
{
    printf("%s dummy function not yet implemented\n", __FUNCTION__);
    return 0;
}

int MRns_name_pton(const char *, u_char *, size_t)
{
    printf("%s dummy function not yet implemented\n", __FUNCTION__);
    return 0;
}

int dns_zone_dereference (struct dns_zone **ptr, const char *file, int line)
{
    printf("%s dummy function not yet implemented\n", __FUNCTION__);
    return 0;
}

isc_result_t enter_dns_zone (struct dns_zone *zone)
{
    printf("%s dummy function not yet implemented\n", __FUNCTION__);
    return (isc_result_t)0;
}