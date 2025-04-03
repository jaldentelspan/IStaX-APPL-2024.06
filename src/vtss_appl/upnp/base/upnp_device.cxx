/*

 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable
 format (e.g. HEX file) and only in or with products utilizing the Microsemi
 switch and PHY products.  The source code of the software may not be
 disclosed, transmitted or distributed without the prior written permission of
 Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all
 ownership, copyright, trade secret and proprietary rights in the software and
 its source code, including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
 WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
 ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
 NON-INFRINGEMENT.

*/

#include "upnp_device.h"
#include <pthread.h>
#include <stdio.h>
#include <upnp/upnp.h>
#include <fcntl.h>
#include "vtss_upnp.h"

//Device handle returned from sdk
extern UpnpDevice_Handle device_handle;

#define DEFAULT_WEB_DIR "./web"
#define DEFAULT_UPNP_MAX_MEM_SHIP "20\n"

/*
   The XML content of device description
 */
char g_xml_content[] =
    "<?xml version=\"1.0\"?>"
    "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
    "<specVersion>"
    "<major>1</major>"
    "<minor>0</minor>"
    "</specVersion>"
    "<device>"
    "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
    "<friendlyName>SMBStaX</friendlyName>"
    "<manufacturer>Microchip</manufacturer>"
    "<manufacturerURL>http://www.microchip.com</manufacturerURL>"
    "<modelDescription>Layer2+ Giga Stacking Switch SMBStaX</modelDescription>"
    "<modelName></modelName>"
    "<modelNumber></modelNumber>"
    "<serialNumber>%u</serialNumber>"
    "<UDN>%s</UDN>"
    "<serviceList>"
    "<service>"
    "<serviceType>urn:schemas-upnp-org:service:Layer4_Layer2:1</serviceType>"
    "<serviceId>urn:upnp-org:serviceId:Layer4_Layer21</serviceId>"
    "<controlURL></controlURL>"
    "<eventSubURL></eventSubURL>"
    "<SCPDURL></SCPDURL>"
    "</service>"
    "</serviceList>"
    "<presentationURL>http://%s:80</presentationURL>"
    "</device>"
    "</root>";

/*
   Device handle supplied by UPnP SDK
 */
UpnpDevice_Handle device_handle = -1;

/******************************************************************************
 * DeviceCallbackEventHandler
 *
 * Description:
 *       The callback handler registered with the SDK while registering
 *       root device.
 *****************************************************************************/
static void
DeviceCallbackEventHandler( Upnp_EventType EventType,
                            void *Event,
                            void *Cookie )
{
    if (EventType) {
        ;
    }
    if (Event) {
        ;
    }
    if (Cookie) {
        ;
    }
}
/******************************************************************************
 * base_upnp_device_start
 *
 * Description:
 *      Initializes the UPnP Sdk, registers the device, and sends out
 *      advertisements.
 *
 * Parameters:
 *
 *   ip_address - ip address to initialize the sdk (may be NULL)
 *                if null, then the first non null loopback address is used.
 *   port       - port number to initialize the sdk (may be 0)
 *                if zero, then a random number is used.
 *   adv_int    - advertisement duration to initialize the sdk (may be 0).
 *                if zero, then default 1800 is used.
 *   udn_str    - Universally Unique Identifier, generate by MAC address.
 *   sn         - The serial number of device.
 *****************************************************************************/
int base_upnp_device_start(char *ip_address, unsigned short port, unsigned long adv_int, char *udn_str, unsigned long sn)
{
    char buff[1024] = {0};
    int ret = UPNP_E_SUCCESS, fd = -1;

    T_D("Enter");

    // Setup the amount of igmp_max_memberships
    fd = open("/proc/sys/net/ipv4/igmp_max_memberships", O_WRONLY);
    if (fd != -1) {
        write(fd, DEFAULT_UPNP_MAX_MEM_SHIP, strlen(DEFAULT_UPNP_MAX_MEM_SHIP));
        close(fd);
    }
    // Give Linux a second to discover the change in
    // igmp_max_memberships before continuing with the upnp
    // configuration
    sleep(1);

    T_I("\n\tInitializing UPnP Sdk with ipaddress = %s port = %u\n",
        ip_address ? ip_address : "{NULL}", port);

    if (UPNP_E_SUCCESS != (ret = UpnpInit2(ip_address, port))) {
        T_E("UpnpInit() error! ret = %d", ret);
        UpnpFinish();
        return ret;
    }

    if ( ip_address == NULL ) {
        ip_address = UpnpGetServerIpAddress();
    }

    port = UpnpGetServerPort();

    T_N("\n\tUPnP Initialized ipaddress = %s, port = %u\n", ip_address ? ip_address : "{NULL}", port);

    if (UPNP_E_SUCCESS != (ret = UpnpSetWebServerRootDir("/tmp/"))) {
        UpnpFinish();
        T_D("UpnpSetWebServerRootDir() error! ret = %d", ret);
        return ret;
    }

    snprintf(buff, sizeof(buff), g_xml_content, sn, udn_str, ip_address);

    if (UPNP_E_SUCCESS != (ret = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC, buff, sizeof(buff), 1,
                                                         (Upnp_FunPtr)DeviceCallbackEventHandler, &device_handle, &device_handle))) {
        T_D("UpnpRegisterRootDevice2() error! ret = %d", ret);
        UpnpFinish();
        return ret;
    } else {
        T_N("\n\tRootDevice Registered\n\tInitializing State Table\n\tState Table Initialized");
        if (UPNP_E_SUCCESS != (ret = UpnpSendAdvertisement( device_handle, adv_int))) {
            T_D("UpnpSendAdvertisement() error! ret = %d", ret);
            UpnpFinish();
            return ret;
        }
    }

    return UPNP_E_SUCCESS;
}

void base_upnp_device_stop(void)
{
    //T_D("Enter");
    UpnpUnRegisterRootDevice(device_handle);
    UpnpFinish();
}
