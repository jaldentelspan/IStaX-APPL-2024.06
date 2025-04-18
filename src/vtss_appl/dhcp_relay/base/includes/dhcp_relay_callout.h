#ifndef _DHCP_RELAY_CALLOUT_H_
#define _DHCP_RELAY_CALLOUT_H_

#ifdef __cplusplus
extern "C" {
#endif

// These must be defined outside of this library.
// They could simply map to their standard counterparts.
void *dhcp_relay_callout_malloc(size_t size);
char *dhcp_relay_callout_strdup(const char *str);
void  dhcp_relay_callout_free(void *ptr);

#ifdef __cplusplus
}
#endif  /* #ifdef __cplusplus */
#endif  /* _DHCP_RELAY_CALLOUT_H_ */

