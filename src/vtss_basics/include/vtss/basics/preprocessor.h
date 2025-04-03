/*
 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef INCLUDE_VTSS_BASICS_PREPROCESSOR_H_
#define INCLUDE_VTSS_BASICS_PREPROCESSOR_H_

#define PP_EXPAND(x) x
#define PP_FOR_EACH_1(F, x, ...) F(x)
#define PP_FOR_EACH_2(F, x, ...) F(x)  PP_EXPAND(PP_FOR_EACH_1(F,  __VA_ARGS__))
#define PP_FOR_EACH_3(F, x, ...) F(x)  PP_EXPAND(PP_FOR_EACH_2(F,  __VA_ARGS__))
#define PP_FOR_EACH_4(F, x, ...) F(x)  PP_EXPAND(PP_FOR_EACH_3(F,  __VA_ARGS__))
#define PP_FOR_EACH_5(F, x, ...) F(x)  PP_EXPAND(PP_FOR_EACH_4(F,  __VA_ARGS__))
#define PP_FOR_EACH_6(F, x, ...) F(x)  PP_EXPAND(PP_FOR_EACH_5(F,  __VA_ARGS__))
#define PP_FOR_EACH_7(F, x, ...) F(x)  PP_EXPAND(PP_FOR_EACH_6(F,  __VA_ARGS__))
#define PP_FOR_EACH_8(F, x, ...) F(x)  PP_EXPAND(PP_FOR_EACH_7(F,  __VA_ARGS__))
#define PP_FOR_EACH_9(F, x, ...) F(x)  PP_EXPAND(PP_FOR_EACH_8(F,  __VA_ARGS__))
#define PP_FOR_EACH_10(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_9(F,  __VA_ARGS__))
#define PP_FOR_EACH_11(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_10(F, __VA_ARGS__))
#define PP_FOR_EACH_12(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_11(F, __VA_ARGS__))
#define PP_FOR_EACH_13(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_12(F, __VA_ARGS__))
#define PP_FOR_EACH_14(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_13(F, __VA_ARGS__))
#define PP_FOR_EACH_15(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_14(F, __VA_ARGS__))
#define PP_FOR_EACH_16(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_15(F, __VA_ARGS__))
#define PP_FOR_EACH_17(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_16(F, __VA_ARGS__))
#define PP_FOR_EACH_18(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_17(F, __VA_ARGS__))
#define PP_FOR_EACH_19(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_18(F, __VA_ARGS__))
#define PP_FOR_EACH_20(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_19(F, __VA_ARGS__))
#define PP_FOR_EACH_21(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_20(F, __VA_ARGS__))
#define PP_FOR_EACH_22(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_21(F, __VA_ARGS__))
#define PP_FOR_EACH_23(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_22(F, __VA_ARGS__))
#define PP_FOR_EACH_24(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_23(F, __VA_ARGS__))
#define PP_FOR_EACH_25(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_24(F, __VA_ARGS__))
#define PP_FOR_EACH_26(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_25(F, __VA_ARGS__))
#define PP_FOR_EACH_27(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_26(F, __VA_ARGS__))
#define PP_FOR_EACH_28(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_27(F, __VA_ARGS__))
#define PP_FOR_EACH_29(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_28(F, __VA_ARGS__))
#define PP_FOR_EACH_30(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_29(F, __VA_ARGS__))
#define PP_FOR_EACH_31(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_30(F, __VA_ARGS__))
#define PP_FOR_EACH_32(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_31(F, __VA_ARGS__))
#define PP_FOR_EACH_33(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_32(F, __VA_ARGS__))
#define PP_FOR_EACH_34(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_33(F, __VA_ARGS__))
#define PP_FOR_EACH_35(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_34(F, __VA_ARGS__))
#define PP_FOR_EACH_36(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_35(F, __VA_ARGS__))
#define PP_FOR_EACH_37(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_36(F, __VA_ARGS__))
#define PP_FOR_EACH_38(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_37(F, __VA_ARGS__))
#define PP_FOR_EACH_39(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_38(F, __VA_ARGS__))
#define PP_FOR_EACH_40(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_39(F, __VA_ARGS__))
#define PP_FOR_EACH_41(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_40(F, __VA_ARGS__))
#define PP_FOR_EACH_42(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_41(F, __VA_ARGS__))
#define PP_FOR_EACH_43(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_42(F, __VA_ARGS__))
#define PP_FOR_EACH_44(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_43(F, __VA_ARGS__))
#define PP_FOR_EACH_45(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_44(F, __VA_ARGS__))
#define PP_FOR_EACH_46(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_45(F, __VA_ARGS__))
#define PP_FOR_EACH_47(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_46(F, __VA_ARGS__))
#define PP_FOR_EACH_48(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_47(F, __VA_ARGS__))
#define PP_FOR_EACH_49(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_48(F, __VA_ARGS__))
#define PP_FOR_EACH_50(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_49(F, __VA_ARGS__))
#define PP_FOR_EACH_51(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_50(F, __VA_ARGS__))
#define PP_FOR_EACH_52(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_51(F, __VA_ARGS__))
#define PP_FOR_EACH_53(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_52(F, __VA_ARGS__))
#define PP_FOR_EACH_54(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_53(F, __VA_ARGS__))
#define PP_FOR_EACH_55(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_54(F, __VA_ARGS__))
#define PP_FOR_EACH_56(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_55(F, __VA_ARGS__))
#define PP_FOR_EACH_57(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_56(F, __VA_ARGS__))
#define PP_FOR_EACH_58(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_57(F, __VA_ARGS__))
#define PP_FOR_EACH_59(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_58(F, __VA_ARGS__))
#define PP_FOR_EACH_60(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_59(F, __VA_ARGS__))
#define PP_FOR_EACH_61(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_60(F, __VA_ARGS__))
#define PP_FOR_EACH_62(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_61(F, __VA_ARGS__))
#define PP_FOR_EACH_63(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_62(F, __VA_ARGS__))
#define PP_FOR_EACH_64(F, x, ...) F(x) PP_EXPAND(PP_FOR_EACH_63(F, __VA_ARGS__))

#define PP_FOR_EACH_NARG(...) \
    PP_FOR_EACH_NARG_(__VA_ARGS__, PP_FOR_EACH_RSEQ_N())
#define PP_FOR_EACH_NARG_(...) PP_EXPAND(PP_FOR_EACH_ARG_N(__VA_ARGS__))
#define PP_FOR_EACH_ARG_N(_1,  _2,  _3,  _4,  _5,  _6,  _7,  _8, \
                           _9, _10, _11, _12, _13, _14, _15, _16, \
                          _17, _18, _19, _20, _21, _22, _23, _24, \
                          _25, _26, _27, _28, _29, _30, _31, _32, \
                          _33, _34, _35, _36, _37, _38, _39, _40, \
                          _41, _42, _43, _44, _45, _46, _47, _48, \
                          _49, _50, _51, _52, _53, _54, _55, _56, \
                          _57, _58, _59, _60, _61, _62, _63, _64, N, ...) N

#define PP_FOR_EACH_RSEQ_N() 64, 63, 62, 61, 60, 59, 58, 57, \
                             56, 55, 54, 53, 52, 51, 50, 49, \
                             48, 47, 46, 45, 44, 43, 42, 41, \
                             40, 39, 38, 37, 36, 35, 34, 33, \
                             32, 31, 30, 29, 28, 27, 26, 25, \
                             24, 23, 22, 21, 20, 19, 18, 17, \
                             16, 15, 14, 13, 12, 11, 10,  9, \
                              8,  7,  6,  5,  4,  3,  2,  1, 0,
#define PP_CONCATENATE(x, y) x##y
#define PP_FOR_EACH_(N, what, ...) \
    PP_EXPAND(PP_CONCATENATE(PP_FOR_EACH_, N)(what, __VA_ARGS__))
#define PP_FOR_EACH(what, ...) \
    PP_FOR_EACH_(PP_FOR_EACH_NARG(__VA_ARGS__), what, __VA_ARGS__)

#define PP_TUPLE_0(_0, ...)  _0
#define PP_TUPLE_1(_0, ...)  PP_TUPLE_0(__VA_ARGS__)
#define PP_TUPLE_2(_0, ...)  PP_TUPLE_1(__VA_ARGS__)
#define PP_TUPLE_3(_0, ...)  PP_TUPLE_2(__VA_ARGS__)
#define PP_TUPLE_4(_0, ...)  PP_TUPLE_3(__VA_ARGS__)
#define PP_TUPLE_5(_0, ...)  PP_TUPLE_4(__VA_ARGS__)
#define PP_TUPLE_6(_0, ...)  PP_TUPLE_5(__VA_ARGS__)
#define PP_TUPLE_7(_0, ...)  PP_TUPLE_6(__VA_ARGS__)
#define PP_TUPLE_8(_0, ...)  PP_TUPLE_7(__VA_ARGS__)
#define PP_TUPLE_9(_0, ...)  PP_TUPLE_8(__VA_ARGS__)
#define PP_TUPLE_10(_0, ...) PP_TUPLE_9(__VA_ARGS__)
#define PP_TUPLE_11(_0, ...) PP_TUPLE_10(__VA_ARGS__)
#define PP_TUPLE_12(_0, ...) PP_TUPLE_11(__VA_ARGS__)
#define PP_TUPLE_13(_0, ...) PP_TUPLE_12(__VA_ARGS__)
#define PP_TUPLE_14(_0, ...) PP_TUPLE_13(__VA_ARGS__)
#define PP_TUPLE_15(_0, ...) PP_TUPLE_14(__VA_ARGS__)
#define PP_TUPLE_16(_0, ...) PP_TUPLE_15(__VA_ARGS__)
#define PP_TUPLE_17(_0, ...) PP_TUPLE_16(__VA_ARGS__)
#define PP_TUPLE_18(_0, ...) PP_TUPLE_17(__VA_ARGS__)
#define PP_TUPLE_19(_0, ...) PP_TUPLE_18(__VA_ARGS__)
#define PP_TUPLE_20(_0, ...) PP_TUPLE_19(__VA_ARGS__)
#define PP_TUPLE_21(_0, ...) PP_TUPLE_20(__VA_ARGS__)
#define PP_TUPLE_22(_0, ...) PP_TUPLE_21(__VA_ARGS__)
#define PP_TUPLE_23(_0, ...) PP_TUPLE_22(__VA_ARGS__)
#define PP_TUPLE_24(_0, ...) PP_TUPLE_23(__VA_ARGS__)
#define PP_TUPLE_25(_0, ...) PP_TUPLE_24(__VA_ARGS__)
#define PP_TUPLE_26(_0, ...) PP_TUPLE_25(__VA_ARGS__)
#define PP_TUPLE_27(_0, ...) PP_TUPLE_26(__VA_ARGS__)
#define PP_TUPLE_28(_0, ...) PP_TUPLE_27(__VA_ARGS__)
#define PP_TUPLE_29(_0, ...) PP_TUPLE_28(__VA_ARGS__)
#define PP_TUPLE_30(_0, ...) PP_TUPLE_29(__VA_ARGS__)
#define PP_TUPLE_31(_0, ...) PP_TUPLE_30(__VA_ARGS__)
#define PP_TUPLE_32(_0, ...) PP_TUPLE_31(__VA_ARGS__)
#define PP_TUPLE_33(_0, ...) PP_TUPLE_32(__VA_ARGS__)
#define PP_TUPLE_34(_0, ...) PP_TUPLE_33(__VA_ARGS__)
#define PP_TUPLE_35(_0, ...) PP_TUPLE_34(__VA_ARGS__)
#define PP_TUPLE_36(_0, ...) PP_TUPLE_35(__VA_ARGS__)
#define PP_TUPLE_37(_0, ...) PP_TUPLE_36(__VA_ARGS__)
#define PP_TUPLE_38(_0, ...) PP_TUPLE_37(__VA_ARGS__)
#define PP_TUPLE_39(_0, ...) PP_TUPLE_38(__VA_ARGS__)
#define PP_TUPLE_40(_0, ...) PP_TUPLE_39(__VA_ARGS__)
#define PP_TUPLE_41(_0, ...) PP_TUPLE_40(__VA_ARGS__)
#define PP_TUPLE_42(_0, ...) PP_TUPLE_41(__VA_ARGS__)
#define PP_TUPLE_43(_0, ...) PP_TUPLE_42(__VA_ARGS__)
#define PP_TUPLE_44(_0, ...) PP_TUPLE_43(__VA_ARGS__)
#define PP_TUPLE_45(_0, ...) PP_TUPLE_44(__VA_ARGS__)
#define PP_TUPLE_46(_0, ...) PP_TUPLE_45(__VA_ARGS__)
#define PP_TUPLE_47(_0, ...) PP_TUPLE_46(__VA_ARGS__)
#define PP_TUPLE_48(_0, ...) PP_TUPLE_47(__VA_ARGS__)
#define PP_TUPLE_49(_0, ...) PP_TUPLE_48(__VA_ARGS__)
#define PP_TUPLE_50(_0, ...) PP_TUPLE_49(__VA_ARGS__)
#define PP_TUPLE_51(_0, ...) PP_TUPLE_50(__VA_ARGS__)
#define PP_TUPLE_52(_0, ...) PP_TUPLE_51(__VA_ARGS__)
#define PP_TUPLE_53(_0, ...) PP_TUPLE_52(__VA_ARGS__)
#define PP_TUPLE_54(_0, ...) PP_TUPLE_53(__VA_ARGS__)
#define PP_TUPLE_55(_0, ...) PP_TUPLE_54(__VA_ARGS__)
#define PP_TUPLE_56(_0, ...) PP_TUPLE_55(__VA_ARGS__)
#define PP_TUPLE_57(_0, ...) PP_TUPLE_56(__VA_ARGS__)
#define PP_TUPLE_58(_0, ...) PP_TUPLE_57(__VA_ARGS__)
#define PP_TUPLE_59(_0, ...) PP_TUPLE_58(__VA_ARGS__)
#define PP_TUPLE_60(_0, ...) PP_TUPLE_59(__VA_ARGS__)
#define PP_TUPLE_61(_0, ...) PP_TUPLE_60(__VA_ARGS__)
#define PP_TUPLE_62(_0, ...) PP_TUPLE_61(__VA_ARGS__)
#define PP_TUPLE_63(_0, ...) PP_TUPLE_62(__VA_ARGS__)
#define PP_TUPLE_64(_0, ...) PP_TUPLE_63(__VA_ARGS__)
#define PP_TUPLE_N(n, t) PP_TUPLE_##n t


#define PP_TUPLE_ARGV_CNT(...)                                                 \
    PP_TUPLE_ARGV_CNT_(__VA_ARGS__, PP_TUPLE_ARGV_CNT_RSEQ_N())

#define PP_TUPLE_ARGV_CNT_(...)                                                \
    PP_EXPAND(PP_TUPLE_ARGV_CNT_ARG_N(__VA_ARGS__))

#define PP_TUPLE_ARGV_CNT_ARG_N(                                               \
         _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,                                \
         _9, _10, _11, _12, _13, _14, _15, _16,                                \
        _17, _18, _19, _20, _21, _22, _23, _24,                                \
        _25, _26, _27, _28, _29, _30, _31, _32,                                \
        _33, _34, _35, _36, _37, _38, _39, _40,                                \
        _41, _42, _43, _44, _45, _46, _47, _48,                                \
        _49, _50, _51, _52, _53, _54, _55, _56,                                \
        _57, _58, _59, _60, _61, _62, _63, _64, N, ...) N

#define PP_TUPLE_ARGV_CNT_RSEQ_N()                                             \
        64, 63, 62, 61, 60, 59, 58, 57,                                        \
        56, 55, 54, 53, 52, 51, 50, 49,                                        \
        48, 47, 46, 45, 44, 43, 42, 41,                                        \
        40, 39, 38, 37, 36, 35, 34, 33,                                        \
        32, 31, 30, 29, 28, 27, 26, 25,                                        \
        24, 23, 22, 21, 20, 19, 18, 17,                                        \
        16, 15, 14, 13, 12, 11, 10,  9,                                        \
         8,  7,  6,  5,  4,  3,  2,  1, 0,

#define PP_TUPLE_OVERLOAD_(N, what, ...) \
    PP_EXPAND(PP_CONCATENATE(what, N)(__VA_ARGS__))

#define PP_TUPLE_OVERLOAD(what, ...) \
    PP_TUPLE_OVERLOAD_(PP_TUPLE_ARGV_CNT(__VA_ARGS__), what, __VA_ARGS__)

#define PP_TUPLE(...) (__VA_ARGS__)

#define VTSS_PP_STRUCT_IMPL_EQUAL__(X)                                         \
    if (X != rhs.X)                                                            \
        return false;
#define VTSS_PP_STRUCT_IMPL_EQUAL_(...)                                        \
    PP_FOR_EACH(VTSS_PP_STRUCT_IMPL_EQUAL__, __VA_ARGS__)                      \
    return true
#define VTSS_PP_STRUCT_IMPL_EQUAL(TUPLE)                                       \
    VTSS_PP_STRUCT_IMPL_EQUAL_ TUPLE

#define VTSS_PP_STRUCT_IMPL_NOT_EQUAL__(X)                                     \
    if (X != rhs.X)                                                            \
        return true;
#define VTSS_PP_STRUCT_IMPL_NOT_EQUAL_(...)                                    \
    PP_FOR_EACH(VTSS_PP_STRUCT_IMPL_NOT_EQUAL__, __VA_ARGS__)                  \
    return false
#define VTSS_PP_STRUCT_IMPL_NOT_EQUAL(TUPLE)                                   \
    VTSS_PP_STRUCT_IMPL_NOT_EQUAL_ TUPLE

#define VTSS_PP_STRUCT_IMPL_ASSIGN__(X)                                        \
    X = rhs.X;
#define VTSS_PP_STRUCT_IMPL_ASSIGN_(...)                                       \
    PP_FOR_EACH(VTSS_PP_STRUCT_IMPL_ASSIGN__, __VA_ARGS__)
#define VTSS_PP_STRUCT_IMPL_ASSIGN(TUPLE)                                      \
    VTSS_PP_STRUCT_IMPL_ASSIGN_ TUPLE

// PP_TUPLE_ARGV_CNT can not detect if there is 0 or 1 argument therefore the
// macro PP_HAS_ARGS checks if it has 0 or multiple arguments
#define PP_HAS_ARGS_IMPL2(                                                     \
         _0,  _1,  _2,  _3,  _4,  _5,  _6,  _7,                                \
         _8,  _9, _10, _11, _12, _13, _14, _15,                                \
        _16, _17, _18, _19, _20, _21, _22, _23,                                \
        _24, _25, _26, _27, _28, _29, _30, _31, N, ...) N
#define PP_HAS_ARGS_SOURCE() PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, \
                             PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, \
                             PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, \
                             PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, \
                             PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, \
                             PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, \
                             PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, \
                             PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_MULTI, PP_HAS_ZERO

#define PP_HAS_ARGS_IMPL(...) PP_HAS_ARGS_IMPL2(__VA_ARGS__)
#define PP_HAS_ARGS(...)      PP_HAS_ARGS_IMPL(__VA_ARGS__, PP_HAS_ARGS_SOURCE())

#define PP_HAS_ZERO false
#define PP_HAS_MULTI true

#define STRINGIFY(X) #X
#define TO_STR(X) STRINGIFY(X)

#define VTSS_ARG_1(a, ...) a
#define VTSS_ARG_2_OR_1(...) VTSS_ARG_2_OR_1_IMPL(__VA_ARGS__, __VA_ARGS__)
#define VTSS_ARG_2_OR_1_IMPL(a, b, ...) b

#endif  // INCLUDE_VTSS_BASICS_PREPROCESSOR_H_
