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

// Warning - this is not a normal header file!
// It should only be used by optional modules to auto-generate most of the
// boilerplate code needed by optional modules.
//
// This header file must:
// - be included in the bottom of the public header of an optional module.
// - outside any name-space scope
// - after the 'PP_MODULE_NAME' and 'PP_MODULE_METHOD_LIST' ' preprocessor
//   symbols has been defined properly.
//
// It will expand to:
// - A struct definition named vtss_appl_module_<PP_MODULE_NAME>_t, which
//   include function pointers to all methods listed by the X-MACRO
//   PP_MODULE_METHOD_LIST.
// - An 'extern void *vtss_appl_module_<PP_MODULE_NAME>;' pointer (which must be
//   declared in vtss_appl/optional_modules/vtss_optional_modules.cxx
// - An alias function in the global namespace for every function included in
//   the PP_MODULE_METHOD_LIST. The alias function will call through the
//   pointer, if the pointer is not null (if it is null it will return an
//   error).


#include "vtss/appl/optional_modules.hxx"

#define PP_MODULE_PTR_NAME VTSS_PP_CAT(VTSS_PP_PTR_PREFIX_NAME, PP_MODULE_NAME)
#define PP_MODULE_TYPE_NAME VTSS_PP_CAT(PP_MODULE_PTR_NAME, \
                                        VTSS_PP_MODULE_TYPE_POSTFIX)

struct PP_MODULE_TYPE_NAME {
#define X(x) decltype(&vtss::appl:: PP_MODULE_NAME ::x) x;
    PP_MODULE_METHOD_LIST
#undef X
};


extern void *PP_MODULE_PTR_NAME;

#define X(x) MODULE_WRAP(x, x, PP_MODULE_TYPE_NAME, PP_MODULE_PTR_NAME, \
                         PP_MODULE_NAME);
PP_MODULE_METHOD_LIST
#undef X

#undef PP_MODULE_NAME
#undef PP_MODULE_PTR_NAME
#undef PP_MODULE_TYPE_NAME
#undef PP_MODULE_METHOD_LIST
