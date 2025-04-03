/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __PORT_LOCK_HXX__
#define __PORT_LOCK_HXX__

struct PortLockScope {
    PortLockScope(const char *file, int line);
    ~PortLockScope(void);

private:
    const char *file;
    const int  line;
};

struct PortUnlockScope {
    PortUnlockScope(const char *file, int line);
    ~PortUnlockScope(void);

private:
    const char *file;
    const int  line;
};

struct PortCallbackLockScope {
    PortCallbackLockScope(const char *file, int line);
    ~PortCallbackLockScope(void);

private:
    const char *file;
    const int  line;
};

#define PORT_LOCK_SCOPE()          PortLockScope         __port_lock_guard__           (__FILE__, __LINE__)
#define PORT_UNLOCK_SCOPE()        PortUnlockScope       __port_unlock_guard__         (__FILE__, __LINE__)
#define PORT_CALLBACK_LOCK_SCOPE() PortCallbackLockScope __port_callback_lock_guard__  (__FILE__, __LINE__)

#endif /* __PORT_LOCK_HXX__ */
