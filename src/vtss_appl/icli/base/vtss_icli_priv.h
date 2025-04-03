/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/*
==============================================================================

    Revision history
    > CP.Wang, 09/11/2013 10:21
        - create

==============================================================================
*/
#ifndef __VTSS_ICLI_PRIV_H__
#define __VTSS_ICLI_PRIV_H__
//****************************************************************************

/*
==============================================================================

    Include File

==============================================================================
*/

/*
==============================================================================

    Constant and Macro

==============================================================================
*/
#define ICLI_PRIV_MATCH_NODE_CNT        16

/*
==============================================================================

    Type Definition

==============================================================================
*/

/*
==============================================================================

    Public Function

==============================================================================
*/
/*
    initialization

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 vtss_icli_priv_init(
    void
);

/*
    set privilege per command

    INPUT
        conf   : privilege command configuration

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 vtss_icli_priv_set(
    IN  icli_priv_cmd_conf_t    *conf
);

/*
    delete privilege per command

    INPUT
        conf : privilege command configuration, index - mode, cmd

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 vtss_icli_priv_delete(
    IN icli_priv_cmd_conf_t    *conf
);

/*
    get first privilege per command

    INPUT
        n/a

    OUTPUT
        conf : first privilege command configuration, index - mode, cmd

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 vtss_icli_priv_get_first(
    OUT icli_priv_cmd_conf_t    *conf
);

/*
    get privilege per command

    INPUT
        conf : privilege command configuration, index - mode, cmd

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 vtss_icli_priv_get(
    INOUT icli_priv_cmd_conf_t    *conf
);

/*
    get next privilege per command
    use index - mode, cmd to find the current one and then get the next one
    of the current one. So, if the current one is not found, then this fails.

    INPUT
        conf : privilege command configuration, sorted by time

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 vtss_icli_priv_get_next(
    INOUT icli_priv_cmd_conf_t    *conf
);

//****************************************************************************
#endif //__VTSS_ICLI_PRIV_H__

