

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Values and constants for the ZLS3072X device internal address map
*
*******************************************************************************/

#ifndef ZL303XX_ADDRESS_MAP_72X_H_
#define ZL303XX_ADDRESS_MAP_72X_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/

#include "zl303xx_Global.h"
#include "zl303xx_AddressMap.h"

/*****************   DEFINES   ************************************************/

/*****************   REGISTER ADDRESS CONSTRUCT   *****************************/

/* Mask to cover all possible size bits:
   - 6 bits allows a range of 0-63 (corresponds to size of 1-64 bytes) */
#define ZL303XX_MEM_SIZE_MASK_72X         (Uint32T)0x0003F000
#define ZL303XX_MEM_SIZE_SHIFT_72X        (Uint16T)12

/* Macro to encode the register SIZE for insertion into a virtual address */
#define ZL303XX_MEM_SIZE_INSERT_72X(bytes)       \
      (Uint32T)((((Uint32T)(bytes) - 1) << ZL303XX_MEM_SIZE_SHIFT_72X)  \
                                                      & ZL303XX_MEM_SIZE_MASK_72X)

/* Address mask, extract & insert definitions */
#define ZL303XX_MEM_ADDR_MASK_72X           (Uint32T)0x00000FFF

/* Macro to make the destination address from a given virtual register address. */
#define ZL303XX_MAKE_MEM_ADDR_72X(fulladdr,size)        \
      (Uint32T)(((fulladdr) & ZL303XX_MEM_ADDR_MASK_72X) |       \
                ZL303XX_MEM_SIZE_INSERT_72X(size))

/* Macro to get the destination address from a given virtual register address. */
#define ZL303XX_MEM_ADDR_EXTRACT_72X(addr)                           \
      (Uint32T)((addr) & ZL303XX_MEM_ADDR_MASK_72X)



/*****************   REGISTER ADDRESS CONSTRUCT   *****************************/

/* Macro to extract the encoded SIZE (in bytes) from a virtual address */
#define ZL303XX_MEM_SIZE_MASK             (Uint32T)0x0003F000
#define ZL303XX_MEM_SIZE_SHIFT            (Uint16T)12

/* Macro to extract the encoded SIZE (in bytes) from a virtual address */
#define ZL303XX_MEM_SIZE_EXTRACT(addr)   \
      (Uint8T)(((addr & ZL303XX_MEM_SIZE_MASK) >> ZL303XX_MEM_SIZE_SHIFT) + 1)


/*****************   DEFINES   ************************************************/

#define ZLS3072X_DPLL_MAX                    3        /* outputs 0-2 */
#define ZLS3072X_NUM_OUTPUTS                 3        /* 3 outputs per device */

/* Dpll reference selection and mode register options */
typedef enum
{
   ZLS3072X_DPLL_MODE_RESET     = 0x0,
   ZLS3072X_DPLL_MODE_FREERUN   = 0x4,
   ZLS3072X_DPLL_MODE_HOLDOVER  = 0x5,
   ZLS3072X_DPLL_MODE_TRACKING  = 0x6,
   ZLS3072X_DPLL_MODE_LAST      = 0xff

} ZLS3072X_DpllModeE;

#define ZLS3072X_CHECK_DPLL_MODE(mode)                      \
           ( (                                              \
               ((mode) == ZLS3072X_DPLL_MODE_FREERUN)   ||  \
               ((mode) == ZLS3072X_DPLL_MODE_HOLDOVER)  ||  \
               ((mode) == ZLS3072X_DPLL_MODE_TRACKING)      \
             )                                              \
            ? ZL303XX_OK : ZL303XX_PARAMETER_INVALID)

#define ZL303XX_CHECK_REF_ID_72X(refId)                       \
           (((zl303xx_RefIdE)(refId) >= ZL303XX_REF_ID_3)   \
               ? ZL303XX_PARAMETER_INVALID                \
               : ZL303XX_OK)

/* Steptime defines */
typedef enum
{
   ZLS3072X_OUTPUT_OC1 = 0,
   ZLS3072X_OUTPUT_OC2,
   ZLS3072X_OUTPUT_OC3,
   ZLS3072X_OUTPUT_LAST
} ZLS3072X_LSDIV_E;

#define ZL303XX_CHECK_LSDIV_NUM(LSDIVNum) \
           (((LSDIVNum) >= ZLS3072X_OUTPUT_LAST) \
                ? ZL303XX_TRACE_ERROR("Invalid LSDIV number=%u", LSDIVNum, 0,0,0,0,0), ZL303XX_PARAMETER_INVALID \
                : ZL303XX_OK)

#define ZLS3072X_LSDIV_PHASE_STEP_MASK  ((1 << ZLS3072X_NUM_OUTPUTS) - 1)

/* ZL3072x configuration file constant */
#define ZLS3072X_NUM_DSP_BYTES              (320*3)


/*****************   REGISTER ADDRESSES   *************************************/

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   REGISTER DECLARATIONS   *************************/

/* Generic 1-byte register */
#define ZLS3072X_1BYTE_REG(reg)             ZL303XX_MAKE_MEM_ADDR_72X(reg, ZL303XX_MEM_SIZE_1_BYTE)

#define REG_EESEL           0x000
#define REG_EEOFFA1         0x001
#define REG_EEOFFA2         0x002
#define REG_EEOFFB1         0x003
#define REG_EEOFFB2         0x004
#define REG_EEOFFC1         0x005
#define REG_EEOFFC2         0x006
#define REG_EEOFFD1         0x007
#define REG_EEOFFD2         0x008
#define REG_MCR1            0x009
#define REG_MCR2            0x00A
#define REG_PLLEN           0x00B
#define REG_ICEN            0x00C
#define REG_OCEN            0x00D
#define REG_GPIOCR1         0x00E
#define REG_GPIOCR2         0x00F
#define REG_GPIO0SS         0x012
#define REG_GPIO1SS         0x013
#define REG_GPIO2SS         0x014
#define REG_GPIO3SS         0x015
#define REG_SCO             0x01A
#define REG_PACR1           0x01B
#define REG_PACR2           0x01C
#define REG_MABCR1          0x01D
#define REG_MABCR2          0x01E
#define REG_MABCR3          0x01F
#define REG_ID1             0x030
#define REG_ID2             0x031
#define REG_DSPID1          0x032
#define REG_DSPID2          0x033
#define REG_CFGSR           0x040
#define REG_GPIOSR          0x041
#define REG_INTSR           0x042
#define REG_GLOBISR         0x043
#define REG_ICISR           0x044
#define REG_OCISR           0x045
#define REG_APLLISR         0x046
#define REG_DPLLISR         0x047
#define REG_APLLSR          0x048
#define REG_PTAB1           0x049
#define REG_PTAB2           0x04A
#define REG_MABSR1          0x04B
#define REG_MABSR2          0x04C
#define REG_PASR            0x04D
#define REG_VALSR1          0x04E
#define REG_VALSR2          0x04F
#define REG_IC1SR           0x050
#define REG_IC2SR           0x051
#define REG_IC3SR           0x052
#define REG_OC1SR           0x053
#define REG_OC2SR           0x054
#define REG_OC3SR           0x055
#define REG_DSRR1           0x056
#define REG_DSRR2           0x057
#define REG_DSRR3           0x058
#define REG_DSRL1           0x059
#define REG_DSRL2           0x05A
#define REG_DSRIE1          0x05B
#define REG_DSRIE2          0x05C
#define REG_DSRB1           0x05D
#define REG_DSRB2           0x05E
#define REG_DSRB3           0x05F
#define REG_DSRB4           0x060
#define REG_DSRB5           0x061
#define REG_DSRB6           0x062
#define REG_DSRB7           0x063
#define REG_DSRB8           0x064
#define REG_DSRB9           0x065
#define REG_DSRB10          0x066
#define REG_DSRB11          0x067
#define REG_DSRB12          0x068
#define REG_DSRB13          0x069
#define REG_DSRB14          0x06A
#define REG_DSRB15          0x06B
#define REG_DSRB16          0x06C
#define REG_DSRB17          0x06D
#define REG_DSRB18          0x06E
#define REG_DSRB19          0x06F
#define REG_DSRB20          0x070
#define REG_DSRB21          0x071
#define REG_DSRB22          0x072
#define REG_DSRB23          0x073
#define REG_DSRB24          0x074
#define REG_DSRB25          0x075
#define REG_DSRB26          0x076
#define REG_DSRB27          0x077
#define REG_DSRB28          0x078
#define REG_DSRB29          0x079
#define REG_DSRB30          0x07A
#define REG_DSRB31          0x07B
#define REG_DSRB32          0x07C
#define REG_DSRB33          0x07D
#define REG_DSRB34          0x07E
#define REG_DSRB35          0x07F
#define REG_DSRB36          0x080
#define REG_DSRB37          0x081
#define REG_DSRB38          0x082
#define REG_DSRB39          0x083
#define REG_DSRB40          0x084
#define REG_DSRB41          0x085
#define REG_DSRB42          0x086
#define REG_DSRB43          0x087
#define REG_DSRB44          0x088
#define REG_DSRB45          0x089
#define REG_DSRB46          0x08A
#define REG_DSRB47          0x08B
#define REG_DSRB48          0x08C
#define REG_DSRB49          0x08D
#define REG_DSRB50          0x08E
#define REG_DSRB51          0x08F
#define REG_DSRB52          0x090
#define REG_DSRB53          0x091
#define REG_DSRB54          0x092
#define REG_DSRB55          0x093
#define REG_DSRB56          0x094
#define REG_DSRB57          0x095
#define REG_DSRB58          0x096
#define REG_DSRB59          0x097
#define REG_DSRB60          0x098
#define REG_DSRB61          0x099
#define REG_DSRB62          0x09A
#define REG_DSRB63          0x09B
#define REG_DSRB64          0x09C
#define REG_DSRB65          0x09D
#define REG_DSRB66          0x09E
#define REG_DSRB67          0x09F
#define REG_DSRB68          0x0A0
#define REG_DSRB69          0x0A1
#define REG_DSRB70          0x0A2
#define REG_DSRB71          0x0A3
#define REG_DSRB72          0x0A4
#define REG_APLLCR1         0x100
#define REG_APLLCR2         0x101
#define REG_APLLCR3         0x102
#define REG_APLLCR4         0x103
#define REG_AFBDIV1         0x106
#define REG_AFBDIV2         0x107
#define REG_AFBDIV3         0x108
#define REG_AFBDIV4         0x109
#define REG_AFBDIV5         0x10A
#define REG_AFBDIV6         0x10B
#define REG_AFBDEN1         0x10C
#define REG_AFBDEN2         0x10D
#define REG_AFBDEN3         0x10E
#define REG_AFBDEN4         0x10F
#define REG_AFBREM1         0x110
#define REG_AFBREM2         0x111
#define REG_AFBREM3         0x112
#define REG_AFBREM4         0x113
#define REG_AFBBP           0x114
#define REG_DCO1            0x120
#define REG_DCO2            0x121
#define REG_DCO3            0x122
#define REG_ALTPLL1         0x123
#define REG_ALTPLL2         0x124
#define REG_PLLTCR5         0x125
#define REG_PLLTCR6         0x126
#define REG_PLLTCR7         0x127
#define REG_PLLTCR8         0x128
#define REG_PLLTCR9         0x129
#define REG_PLLTCR10        0x12A
#define REG_PLLTCR11        0x12B
#define REG_PLLTCR12        0x12C
#define REG_OC1CR1          0x200
#define REG_OC1CR2          0x201
#define REG_OC1CR3          0x202
#define REG_OC1DIV1         0x203
#define REG_OC1DIV2         0x204
#define REG_OC1DIV3         0x205
#define REG_OC1DC           0x206
#define REG_OC1PH           0x207
#define REG_OC1STOP         0x208
#define REG_OC2CR1          0x210
#define REG_OC2CR2          0x211
#define REG_OC2CR3          0x212
#define REG_OC2DIV1         0x213
#define REG_OC2DIV2         0x214
#define REG_OC2DIV3         0x215
#define REG_OC2DC           0x216
#define REG_OC2PH           0x217
#define REG_OC2STOP         0x218
#define REG_OC3CR1          0x220
#define REG_OC3CR2          0x221
#define REG_OC3CR3          0x222
#define REG_OC3DIV1         0x223
#define REG_OC3DIV2         0x224
#define REG_OC3DIV3         0x225
#define REG_OC3DC           0x226
#define REG_OC3PH           0x227
#define REG_OC3STOP         0x228
#define REG_IC1CR1          0x300
#define REG_IC1CR2          0x301
#define REG_IC1CR3          0x302
#define REG_IC1CR4          0x303
#define REG_MON1CR1         0x310
#define REG_MON1CR2         0x311
#define REG_MON1CR3         0x312
#define REG_MON1CR4         0x313
#define REG_MON1CR5         0x314
#define REG_MON1CR6         0x315
#define REG_MON1CR7         0x316
#define REG_MON1CR8         0x317
#define REG_MON1CR9         0x318
#define REG_MON1CR10        0x319
#define REG_MON1CR11        0x31A
#define REG_MON1CR12        0x31B
#define REG_MON1CR13        0x31C
#define REG_IC2CR1          0x320
#define REG_IC2CR2          0x321
#define REG_IC2CR3          0x322
#define REG_IC2CR4          0x323
#define REG_MON2CR1         0x330
#define REG_MON2CR2         0x331
#define REG_MON2CR3         0x332
#define REG_MON2CR4         0x333
#define REG_MON2CR5         0x334
#define REG_MON2CR6         0x335
#define REG_MON2CR7         0x336
#define REG_MON2CR8         0x337
#define REG_MON2CR9         0x338
#define REG_MON2CR10        0x339
#define REG_MON2CR11        0x33A
#define REG_MON2CR12        0x33B
#define REG_MON2CR13        0x33C
#define REG_IC3CR1          0x340
#define REG_IC3CR2          0x341
#define REG_IC3CR3          0x342
#define REG_IC3CR4          0x343
#define REG_MON3CR1         0x350
#define REG_MON3CR2         0x351
#define REG_MON3CR3         0x352
#define REG_MON3CR4         0x353
#define REG_MON3CR5         0x354
#define REG_MON3CR6         0x355
#define REG_MON3CR7         0x356
#define REG_MON3CR8         0x357
#define REG_MON3CR9         0x358
#define REG_MON3CR10        0x359
#define REG_MON3CR11        0x35A
#define REG_MON3CR12        0x35B
#define REG_MON3CR13        0x35C
#define REG_ICSCR1          0x400
#define REG_VALCR1          0x401
#define REG_IPR1            0x402
#define REG_IPR2            0x403
#define REG_PHLKTO          0x404
#define REG_LKATO           0x405
#define REG_DFBSCL1         0x406
#define REG_DFBSCL2         0x407
#define REG_DFBDIV1         0x408
#define REG_DFBDIV2         0x409
#define REG_DFBDIV3         0x40A
#define REG_DPLLCR1         0x40B
#define REG_DPLLCR2         0x40C
#define REG_DPLLCR3         0x40D
#define REG_DALGWR          0x40E
#define REG_DALGRD          0x40F
#define REG_DPCNT1          0x410
#define REG_DPCNT2          0x411
#define REG_DCRB1           0x420
#define REG_DCRB2           0x421
#define REG_DCRB3           0x422
#define REG_DCRB4           0x423
#define REG_DCRB5           0x424
#define REG_DCRB6           0x425
#define REG_DCRB7           0x426
#define REG_DCRB8           0x427
#define REG_DCRB9           0x428
#define REG_DCRB10          0x429
#define REG_DCRB11          0x42A
#define REG_DCRB12          0x42B
#define REG_DCRB13          0x42C
#define REG_DCRB14          0x42D
#define REG_DCRB15          0x42E
#define REG_DCRB16          0x42F
#define REG_DCRB17          0x430
#define REG_DCRB18          0x431
#define REG_DCRB19          0x432
#define REG_DCRB20          0x433
#define REG_DCRB21          0x434
#define REG_DCRB22          0x435
#define REG_DCRB23          0x436
#define REG_DCRB24          0x437
#define REG_DCRB25          0x438
#define REG_DCRB26          0x439
#define REG_DCRB27          0x43A
#define REG_DCRB28          0x43B
#define REG_DCRB29          0x43C
#define REG_DCRB30          0x43D
#define REG_DCRB31          0x43E
#define REG_DCRB32          0x43F
#define REG_DCRB33          0x440
#define REG_DCRB34          0x441
#define REG_DCRB35          0x442
#define REG_DCRB36          0x443
#define REG_DCRB37          0x444
#define REG_DCRB38          0x445
#define REG_DCRB39          0x446
#define REG_DCRB40          0x447
#define REG_DCRB41          0x448
#define REG_DCRB42          0x449
#define REG_DCRB43          0x44A
#define REG_DCRB44          0x44B
#define REG_DCRB45          0x44C
#define REG_DCRB46          0x44D
#define REG_DCRB47          0x44E
#define REG_DCRB48          0x44F
#define REG_DCRB49          0x450
#define REG_DCRB50          0x451
#define REG_DCRB51          0x452
#define REG_DCRB52          0x453
#define REG_DCRB53          0x454
#define REG_DCRB54          0x455
#define REG_DCRB55          0x456
#define REG_DCRB56          0x457
#define REG_DCRB57          0x458
#define REG_DCRB58          0x459
#define REG_DCRB59          0x45A
#define REG_DCRB60          0x45B
#define REG_DCRB61          0x45C
#define REG_DCRB62          0x45D
#define REG_DCRB63          0x45E
#define REG_DCRB64          0x45F
#define REG_DCRB65          0x460
#define REG_DCRB66          0x461
#define REG_DCRB67          0x462
#define REG_DCRB68          0x463
#define REG_DCRB69          0x464
#define REG_DCRB70          0x465
#define REG_DCRB71          0x466
#define REG_DCRB72          0x467
#define REG_TST_RST         0x600
#define REG_GTEST3          0x601
#define REG_TST_CLK         0x602
#define REG_D1PFD           0x603
#define REG_DPLL1GPIO       0x604
#define REG_DPLL2GPIO       0x605
#define REG_DCO1GPIO        0x606
#define REG_DCO2GPIO        0x607
#define REG_UP1GPIO         0x608
#define REG_UP2GPIO         0x609
#define REG_GLB1GPIO        0x60A
#define REG_GLB2GPIO        0x60B
#define REG_TST3GPIO        0x60C
#define REG_TST4GPIO        0x60D
#define REG_HWTST1          0x60E
#define REG_HWTST2          0x60F
#define REG_AFREQ1          0x610
#define REG_PSJAC1          0x611
#define REG_PSJAC3          0x612
#define REG_PSJAS1          0x613
#define REG_PSJAS2          0x614
#define REG_PSJAS3          0x615
#define REG_ASR1            0x616
#define REG_ALSR1           0x617
#define REG_AIER1           0x618
#define REG_TRAN1           0x619
#define REG_TRAN2           0x61A
#define REG_TRAN3           0x61B
#define REG_PLLTCR13        0x61C
#define REG_PLLTCR14        0x61D
#define REG_PLLTCR15        0x61E
#define REG_PLLTCR16        0x61F
#define REG_PLLTCR17        0x620
#define REG_PLLTCR18        0x621
#define REG_TST1CR1         0x622
#define REG_TST2CR1         0x623
#define REG_TST3CR1         0x624
#define REG_TST_FB          0x625
#define REG_TST_IC          0x626
#define REG_IC1TSR          0x627
#define REG_IC2TSR          0x628
#define REG_IC3TSR          0x629
#define REG_OC1TCR          0x62A
#define REG_OC2TCR          0x62B
#define REG_OC3TCR          0x62C
#define REG_XO_TEST         0x62D
#define REG_REF_IO          0x62E
#define REG_TCCR1           0x630
#define REG_TCCR2           0x631
#define REG_TCCR3           0x632
#define REG_0633            0x633
#define REG_0634            0x634
#define REG_0635            0x635
#define REG_0636            0x636
#define REG_0637            0x637
#define REG_0638            0x638
#define REG_0639            0x639
#define REG_063A            0x63A
#define REG_063B            0x63B
#define REG_063C            0x63C
#define REG_063D            0x63D
#define REG_063E            0x63E
#define REG_063F            0x63F
#define REG_0640            0x640
#define REG_0641            0x641
#define REG_0642            0x642
#define REG_0643            0x643
#define REG_0644            0x644
#define REG_0645            0x645
#define REG_0646            0x646
#define REG_0647            0x647
#define REG_0648            0x648
#define REG_0649            0x649
#define REG_064A            0x64A
#define REG_064B            0x64B
#define REG_064C            0x64C
#define REG_064D            0x64D
#define REG_064E            0x64E
#define REG_064F            0x64F
#define REG_0650            0x650
#define REG_0651            0x651
#define REG_0652            0x652
#define REG_0653            0x653
#define REG_0654            0x654
#define REG_0655            0x655
#define REG_0656            0x656
#define REG_0657            0x657
#define REG_0658            0x658
#define REG_0659            0x659
#define REG_065A            0x65A
#define REG_065B            0x65B
#define REG_065C            0x65C
#define REG_065D            0x65D
#define REG_065E            0x65E
#define REG_065F            0x65F
#define REG_0660            0x660
#define REG_0661            0x661
#define REG_0662            0x662
#define REG_0663            0x663
#define REG_0664            0x664
#define REG_0665            0x665
#define REG_0666            0x666
#define REG_0667            0x667
#define REG_0668            0x668
#define REG_0669            0x669
#define REG_066A            0x66A
#define REG_066B            0x66B
#define REG_066C            0x66C
#define REG_066D            0x66D
#define REG_066E            0x66E
#define REG_066F            0x66F
#define REG_0670            0x670
#define REG_0671            0x671
#define REG_0672            0x672
#define REG_0673            0x673
#define REG_0674            0x674
#define REG_0675            0x675
#define REG_0676            0x676
#define REG_0677            0x677
#define REG_0678            0x678
#define REG_0679            0x679
#define REG_067A            0x67A
#define REG_067B            0x67B
#define REG_067C            0x67C
#define REG_067D            0x67D
#define REG_067E            0x67E
#define REG_067F            0x67F
#define REG_0680            0x680
#define REG_0681            0x681
#define REG_0682            0x682
#define REG_0683            0x683
#define REG_0684            0x684
#define REG_0685            0x685
#define REG_0686            0x686
#define REG_0687            0x687
#define REG_0688            0x688
#define REG_0689            0x689
#define REG_068A            0x68A
#define REG_068B            0x68B
#define REG_068C            0x68C
#define REG_068D            0x68D
#define REG_068E            0x68E
#define REG_068F            0x68F
#define REG_0690            0x690
#define REG_0691            0x691
#define REG_0692            0x692
#define REG_0693            0x693
#define REG_0694            0x694
#define REG_0695            0x695
#define REG_0696            0x696
#define REG_0697            0x697
#define REG_0698            0x698
#define REG_0699            0x699
#define REG_069A            0x69A
#define REG_069B            0x69B
#define REG_069C            0x69C
#define REG_069D            0x69D
#define REG_069E            0x69E
#define REG_069F            0x69F
#define REG_06A0            0x6A0
#define REG_06A1            0x6A1
#define REG_06A2            0x6A2
#define REG_06A3            0x6A3
#define REG_06A4            0x6A4
#define REG_06A5            0x6A5
#define REG_06A6            0x6A6
#define REG_06A7            0x6A7
#define REG_06A8            0x6A8
#define REG_06A9            0x6A9
#define REG_06AA            0x6AA
#define REG_06AB            0x6AB
#define REG_06AC            0x6AC
#define REG_06AD            0x6AD
#define REG_06AE            0x6AE
#define REG_06AF            0x6AF
#define REG_06B0            0x6B0
#define REG_06B1            0x6B1
#define REG_06B2            0x6B2
#define REG_06B3            0x6B3
#define REG_06B4            0x6B4
#define REG_06B5            0x6B5
#define REG_06B6            0x6B6
#define REG_06B7            0x6B7
#define REG_06B8            0x6B8
#define REG_06B9            0x6B9
#define REG_06BA            0x6BA
#define REG_06BB            0x6BB
#define REG_06BC            0x6BC
#define REG_06BD            0x6BD
#define REG_06BE            0x6BE
#define REG_06BF            0x6BF
#define REG_06C0            0x6C0
#define REG_06C1            0x6C1
#define REG_06C2            0x6C2
#define REG_06C3            0x6C3
#define REG_06C4            0x6C4
#define REG_06C5            0x6C5
#define REG_06C6            0x6C6
#define REG_06C7            0x6C7
#define REG_06C8            0x6C8
#define REG_06C9            0x6C9
#define REG_06CA            0x6CA
#define REG_06CB            0x6CB
#define REG_06CC            0x6CC
#define REG_06CD            0x6CD
#define REG_06CE            0x6CE
#define REG_06CF            0x6CF
#define REG_06D0            0x6D0
#define REG_06D1            0x6D1
#define REG_06D2            0x6D2
#define REG_06D3            0x6D3
#define REG_06D4            0x6D4
#define REG_06D5            0x6D5
#define REG_06D6            0x6D6
#define REG_06D7            0x6D7
#define REG_06D8            0x6D8
#define REG_06D9            0x6D9
#define REG_06DA            0x6DA
#define REG_06DB            0x6DB
#define REG_06DC            0x6DC
#define REG_06DD            0x6DD
#define REG_06DE            0x6DE
#define REG_06DF            0x6DF
#define REG_06E0            0x6E0
#define REG_06E1            0x6E1
#define REG_06E2            0x6E2
#define REG_06E3            0x6E3
#define REG_06E4            0x6E4
#define REG_06E5            0x6E5
#define REG_06E6            0x6E6
#define REG_06E7            0x6E7
#define REG_06E8            0x6E8
#define REG_06E9            0x6E9
#define REG_06EA            0x6EA
#define REG_06EB            0x6EB
#define REG_06EC            0x6EC
#define REG_06ED            0x6ED
#define REG_06EE            0x6EE
#define REG_06EF            0x6EF
#define REG_06F0            0x6F0
#define REG_06F1            0x6F1
#define REG_06F2            0x6F2
#define REG_06F3            0x6F3
#define REG_06F4            0x6F4
#define REG_06F5            0x6F5
#define REG_06F6            0x6F6
#define REG_06F7            0x6F7
#define REG_06F8            0x6F8
#define REG_06F9            0x6F9
#define REG_06FA            0x6FA
#define REG_06FB            0x6FB
#define REG_06FC            0x6FC
#define REG_06FD            0x6FD
#define REG_06FE            0x6FE
#define REG_06FF            0x6FF

/* DSP defined regs */
#define REG_DFREQZ1         REG_DCRB1
#define REG_DFREQZ2         REG_DCRB2
#define REG_DFREQZ3         REG_DCRB3
#define REG_DFREQZ4         REG_DCRB4
#define REG_DFREQZ5         REG_DCRB5

#define REG_DPHOFF1         REG_DCRB9
#define REG_DPHOFF2         REG_DCRB10
#define REG_DPHOFF3         REG_DCRB11
#define REG_DPHOFF4         REG_DCRB12

#define REG_DFREQ1          REG_DSRB9
#define REG_DFREQ2          REG_DSRB10
#define REG_DFREQ3          REG_DSRB11
#define REG_DFREQ4          REG_DSRB12

#define REG_DPHASE1         REG_DSRB33
#define REG_DPHASE2         REG_DSRB34
#define REG_DPHASE3         REG_DSRB35
#define REG_DPHASE4         REG_DSRB36


#define ZLS3072X_EESEL_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_EESEL     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_EEOFFA1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_EEOFFA1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_EEOFFA2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_EEOFFA2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_EEOFFB1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_EEOFFB1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_EEOFFB2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_EEOFFB2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_EEOFFC1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_EEOFFC1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_EEOFFC2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_EEOFFC2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_EEOFFD1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_EEOFFD1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_EEOFFD2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_EEOFFD2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MCR1_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_MCR1      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MCR2_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_MCR2      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLEN_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLEN     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_ICEN_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_ICEN      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OCEN_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_OCEN      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_GPIOCR1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_GPIOCR1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_GPIOCR2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_GPIOCR2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_GPIO0SS_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_GPIO0SS   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_GPIO1SS_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_GPIO1SS   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_GPIO2SS_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_GPIO2SS   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_GPIO3SS_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_GPIO3SS   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_SCO_REG            ZL303XX_MAKE_MEM_ADDR_72X(REG_SCO       ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PACR1_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_PACR1     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PACR2_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_PACR2     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MABCR1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_MABCR1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MABCR2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_MABCR2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MABCR3_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_MABCR3    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_ID1_REG            ZL303XX_MAKE_MEM_ADDR_72X(REG_ID1       ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_ID2_REG            ZL303XX_MAKE_MEM_ADDR_72X(REG_ID2       ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSPID1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSPID1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSPID2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSPID2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_CFGSR_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_CFGSR     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_GPIOSR_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_GPIOSR    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_INTSR_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_INTSR     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_GLOBISR_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_GLOBISR   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_ICISR_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_ICISR     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OCISR_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_OCISR     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_APLLISR_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_APLLISR   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DPLLISR_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DPLLISR   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_APLLSR_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_APLLSR    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PTAB1_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_PTAB1     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PTAB2_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_PTAB2     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MABSR1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_MABSR1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MABSR2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_MABSR2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PASR_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_PASR      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_VALSR1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_VALSR1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_VALSR2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_VALSR2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC1SR_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_IC1SR     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC2SR_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_IC2SR     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC3SR_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_IC3SR     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC1SR_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_OC1SR     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC2SR_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_OC2SR     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC3SR_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_OC3SR     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRR1_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRR1     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRR2_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRR2     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRR3_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRR3     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRL1_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRL1     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRL2_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRL2     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRIE1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRIE1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRIE2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRIE2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB1_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB1     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB2_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB2     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB3_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB3     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB4_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB4     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB5_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB5     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB6_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB6     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB7_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB7     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB8_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB8     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB9_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB9     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB10_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB10    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB11_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB11    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB12_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB12    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB13_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB13    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB14_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB14    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB15_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB15    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB16_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB16    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB17_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB17    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB18_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB18    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB19_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB19    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB20_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB20    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB21_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB21    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB22_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB22    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB23_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB23    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB24_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB24    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB25_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB25    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB26_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB26    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB27_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB27    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB28_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB28    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB29_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB29    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB30_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB30    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB31_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB31    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB32_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB32    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB33_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB33    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB34_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB34    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB35_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB35    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB36_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB36    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB37_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB37    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB38_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB38    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB39_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB39    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB40_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB40    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB41_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB41    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB42_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB42    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB43_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB43    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB44_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB44    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB45_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB45    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB46_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB46    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB47_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB47    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB48_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB48    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB49_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB49    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB50_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB50    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB51_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB51    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB52_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB52    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB53_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB53    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB54_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB54    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB55_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB55    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB56_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB56    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB57_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB57    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB58_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB58    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB59_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB59    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB60_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB60    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB61_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB61    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB62_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB62    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB63_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB63    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB64_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB64    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB65_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB65    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB66_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB66    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB67_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB67    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB68_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB68    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB69_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB69    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB70_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB70    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB71_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB71    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DSRB72_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DSRB72    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_APLLCR1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_APLLCR1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_APLLCR2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_APLLCR2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_APLLCR3_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_APLLCR3   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_APLLCR4_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_APLLCR4   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBDIV1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBDIV1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBDIV2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBDIV2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBDIV3_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBDIV3   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBDIV4_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBDIV4   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBDIV5_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBDIV5   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBDIV6_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBDIV6   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBDEN1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBDEN1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBDEN2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBDEN2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBDEN3_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBDEN3   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBDEN4_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBDEN4   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBREM1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBREM1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBREM2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBREM2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBREM3_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBREM3   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBREM4_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBREM4   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFBBP_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_AFBBP     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCO1_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_DCO1      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCO2_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_DCO2      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCO3_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_DCO3      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_ALTPLL1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_ALTPLL1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_ALTPLL2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_ALTPLL2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR5_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR5   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR6_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR6   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR7_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR7   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR8_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR8   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR9_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR9   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR10_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR10  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR11_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR11  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR12_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR12  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC1CR1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_OC1CR1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC1CR2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_OC1CR2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC1CR3_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_OC1CR3    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC1DIV1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_OC1DIV1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC1DIV2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_OC1DIV2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC1DIV3_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_OC1DIV3   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC1DC_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_OC1DC     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC1PH_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_OC1PH     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC1STOP_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_OC1STOP   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC2CR1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_OC2CR1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC2CR2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_OC2CR2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC2CR3_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_OC2CR3    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC2DIV1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_OC2DIV1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC2DIV2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_OC2DIV2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC2DIV3_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_OC2DIV3   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC2DC_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_OC2DC     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC2PH_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_OC2PH     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC2STOP_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_OC2STOP   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC3CR1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_OC3CR1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC3CR2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_OC3CR2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC3CR3_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_OC3CR3    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC3DIV1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_OC3DIV1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC3DIV2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_OC3DIV2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC3DIV3_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_OC3DIV3   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC3DC_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_OC3DC     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC3PH_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_OC3PH     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC3STOP_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_OC3STOP   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC1CR1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC1CR1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC1CR2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC1CR2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC1CR3_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC1CR3    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC1CR4_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC1CR4    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR3_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR3   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR4_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR4   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR5_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR5   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR6_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR6   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR7_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR7   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR8_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR8   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR9_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR9   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR10_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR10  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR11_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR11  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR12_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR12  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON1CR13_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_MON1CR13  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC2CR1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC2CR1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC2CR2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC2CR2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC2CR3_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC2CR3    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC2CR4_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC2CR4    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR3_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR3   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR4_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR4   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR5_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR5   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR6_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR6   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR7_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR7   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR8_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR8   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR9_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR9   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR10_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR10  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR11_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR11  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR12_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR12  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON2CR13_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_MON2CR13  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC3CR1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC3CR1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC3CR2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC3CR2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC3CR3_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC3CR3    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC3CR4_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC3CR4    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR3_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR3   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR4_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR4   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR5_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR5   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR6_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR6   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR7_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR7   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR8_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR8   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR9_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR9   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR10_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR10  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR11_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR11  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR12_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR12  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_MON3CR13_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_MON3CR13  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_ICSCR1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_ICSCR1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_VALCR1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_VALCR1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IPR1_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_IPR1      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IPR2_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_IPR2      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PHLKTO_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_PHLKTO    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_LKATO_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_LKATO     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DFBSCL1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DFBSCL1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DFBSCL2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DFBSCL2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DFBDIV1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DFBDIV1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DFBDIV2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DFBDIV2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DFBDIV3_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DFBDIV3   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DPLLCR1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DPLLCR1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DPLLCR2_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DPLLCR2   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DPLLCR3_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DPLLCR3   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DALGWR_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DALGWR    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DALGWR_REG_2BYTE   ZL303XX_MAKE_MEM_ADDR_72X(REG_DALGWR    ,      ZL303XX_MEM_SIZE_2_BYTE)
#define ZLS3072X_DALGRD_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DALGRD    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DPCNT1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DPCNT1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DPCNT2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DPCNT2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB1_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB1     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB2_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB2     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB3_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB3     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB4_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB4     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB5_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB5     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB6_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB6     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB7_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB7     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB8_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB8     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB9_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB9     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB10_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB10    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB11_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB11    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB12_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB12    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB13_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB13    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB14_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB14    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB15_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB15    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB16_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB16    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB17_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB17    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB18_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB18    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB19_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB19    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB20_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB20    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB21_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB21    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB22_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB22    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB23_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB23    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB24_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB24    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB25_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB25    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB26_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB26    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB27_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB27    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB28_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB28    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB29_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB29    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB30_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB30    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB31_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB31    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB32_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB32    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB33_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB33    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB34_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB34    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB35_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB35    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB36_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB36    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB37_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB37    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB38_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB38    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB39_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB39    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB40_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB40    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB41_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB41    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB42_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB42    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB43_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB43    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB44_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB44    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB45_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB45    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB46_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB46    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB47_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB47    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB48_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB48    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB49_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB49    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB50_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB50    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB51_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB51    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB52_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB52    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB53_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB53    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB54_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB54    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB55_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB55    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB56_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB56    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB57_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB57    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB58_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB58    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB59_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB59    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB60_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB60    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB61_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB61    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB62_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB62    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB63_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB63    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB64_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB64    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB65_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB65    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB66_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB66    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB67_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB67    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB68_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB68    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB69_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB69    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB70_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB70    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB71_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB71    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCRB72_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB72    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TST_RST_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_TST_RST   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_GTEST3_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_GTEST3    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TST_CLK_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_TST_CLK   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_D1PFD_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_D1PFD     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DPLL1GPIO_REG      ZL303XX_MAKE_MEM_ADDR_72X(REG_DPLL1GPIO ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DPLL2GPIO_REG      ZL303XX_MAKE_MEM_ADDR_72X(REG_DPLL2GPIO ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCO1GPIO_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_DCO1GPIO  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DCO2GPIO_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_DCO2GPIO  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_UP1GPIO_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_UP1GPIO   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_UP2GPIO_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_UP2GPIO   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_GLB1GPIO_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_GLB1GPIO  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_GLB2GPIO_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_GLB2GPIO  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TST3GPIO_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_TST3GPIO  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TST4GPIO_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_TST4GPIO  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_HWTST1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_HWTST1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_HWTST2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_HWTST2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AFREQ1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_AFREQ1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PSJAC1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_PSJAC1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PSJAC3_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_PSJAC3    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PSJAS1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_PSJAS1    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PSJAS2_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_PSJAS2    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PSJAS3_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_PSJAS3    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_ASR1_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_ASR1      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_ALSR1_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_ALSR1     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_AIER1_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_AIER1     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TRAN1_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_TRAN1     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TRAN2_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_TRAN2     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TRAN3_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_TRAN3     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR13_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR13  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR14_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR14  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR15_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR15  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR16_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR16  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR17_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR17  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_PLLTCR18_REG       ZL303XX_MAKE_MEM_ADDR_72X(REG_PLLTCR18  ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TST1CR1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_TST1CR1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TST2CR1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_TST2CR1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TST3CR1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_TST3CR1   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TST_FB_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_TST_FB    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TST_IC_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_TST_IC    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC1TSR_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC1TSR    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC2TSR_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC2TSR    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_IC3TSR_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_IC3TSR    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC1TCR_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_OC1TCR    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC2TCR_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_OC2TCR    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_OC3TCR_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_OC3TCR    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_XO_TEST_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_XO_TEST   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_REF_IO_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_REF_IO    ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TCCR1_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_TCCR1     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TCCR2_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_TCCR2     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_TCCR3_REG          ZL303XX_MAKE_MEM_ADDR_72X(REG_TCCR3     ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0633_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0633      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0634_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0634      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0635_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0635      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0636_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0636      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0637_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0637      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0638_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0638      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0639_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0639      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_063A_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_063A      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_063B_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_063B      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_063C_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_063C      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_063D_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_063D      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_063E_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_063E      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_063F_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_063F      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0640_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0640      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0641_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0641      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0642_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0642      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0643_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0643      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0644_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0644      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0645_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0645      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0646_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0646      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0647_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0647      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0648_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0648      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0649_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0649      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_064A_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_064A      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_064B_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_064B      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_064C_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_064C      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_064D_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_064D      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_064E_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_064E      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_064F_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_064F      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0650_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0650      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0651_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0651      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0652_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0652      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0653_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0653      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0654_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0654      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0655_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0655      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0656_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0656      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0657_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0657      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0658_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0658      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0659_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0659      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_065A_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_065A      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_065B_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_065B      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_065C_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_065C      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_065D_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_065D      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_065E_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_065E      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_065F_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_065F      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0660_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0660      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0661_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0661      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0662_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0662      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0663_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0663      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0664_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0664      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0665_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0665      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0666_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0666      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0667_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0667      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0668_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0668      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0669_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0669      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_066A_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_066A      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_066B_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_066B      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_066C_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_066C      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_066D_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_066D      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_066E_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_066E      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_066F_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_066F      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0670_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0670      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0671_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0671      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0672_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0672      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0673_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0673      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0674_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0674      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0675_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0675      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0676_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0676      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0677_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0677      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0678_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0678      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0679_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0679      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_067A_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_067A      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_067B_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_067B      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_067C_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_067C      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_067D_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_067D      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_067E_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_067E      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_067F_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_067F      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0680_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0680      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0681_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0681      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0682_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0682      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0683_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0683      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0684_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0684      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0685_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0685      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0686_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0686      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0687_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0687      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0688_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0688      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0689_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0689      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_068A_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_068A      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_068B_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_068B      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_068C_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_068C      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_068D_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_068D      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_068E_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_068E      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_068F_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_068F      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0690_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0690      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0691_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0691      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0692_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0692      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0693_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0693      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0694_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0694      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0695_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0695      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0696_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0696      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0697_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0697      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0698_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0698      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_0699_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_0699      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_069A_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_069A      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_069B_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_069B      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_069C_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_069C      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_069D_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_069D      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_069E_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_069E      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_069F_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_069F      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06A0_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06A0      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06A1_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06A1      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06A2_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06A2      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06A3_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06A3      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06A4_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06A4      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06A5_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06A5      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06A6_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06A6      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06A7_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06A7      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06A8_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06A8      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06A9_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06A9      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06AA_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06AA      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06AB_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06AB      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06AC_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06AC      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06AD_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06AD      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06AE_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06AE      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06AF_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06AF      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06B0_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06B0      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06B1_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06B1      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06B2_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06B2      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06B3_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06B3      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06B4_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06B4      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06B5_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06B5      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06B6_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06B6      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06B7_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06B7      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06B8_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06B8      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06B9_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06B9      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06BA_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06BA      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06BB_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06BB      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06BC_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06BC      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06BD_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06BD      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06BE_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06BE      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06BF_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06BF      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06C0_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06C0      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06C1_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06C1      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06C2_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06C2      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06C3_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06C3      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06C4_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06C4      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06C5_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06C5      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06C6_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06C6      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06C7_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06C7      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06C8_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06C8      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06C9_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06C9      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06CA_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06CA      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06CB_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06CB      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06CC_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06CC      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06CD_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06CD      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06CE_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06CE      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06CF_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06CF      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06D0_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06D0      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06D1_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06D1      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06D2_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06D2      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06D3_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06D3      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06D4_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06D4      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06D5_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06D5      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06D6_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06D6      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06D7_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06D7      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06D8_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06D8      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06D9_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06D9      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06DA_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06DA      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06DB_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06DB      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06DC_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06DC      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06DD_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06DD      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06DE_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06DE      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06DF_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06DF      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06E0_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06E0      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06E1_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06E1      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06E2_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06E2      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06E3_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06E3      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06E4_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06E4      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06E5_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06E5      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06E6_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06E6      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06E7_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06E7      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06E8_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06E8      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06E9_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06E9      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06EA_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06EA      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06EB_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06EB      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06EC_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06EC      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06ED_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06ED      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06EE_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06EE      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06EF_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06EF      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06F0_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06F0      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06F1_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06F1      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06F2_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06F2      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06F3_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06F3      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06F4_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06F4      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06F5_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06F5      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06F6_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06F6      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06F7_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06F7      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06F8_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06F8      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06F9_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06F9      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06FA_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06FA      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06FB_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06FB      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06FC_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06FC      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06FD_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06FD      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06FE_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06FE      ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_06FF_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_06FF      ,      ZL303XX_MEM_SIZE_1_BYTE)

/* Multi-byte registers */
#define ZLS3072X_DFREQZ5_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DFREQZ5   ,      ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3072X_DFREQZ1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DFREQZ1   ,      ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3072X_DPHOFF1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DPHOFF1   ,      ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3072X_DFREQ1_REG         ZL303XX_MAKE_MEM_ADDR_72X(REG_DFREQ1    ,      ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3072X_DPHASE1_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DPHASE1   ,      ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3072X_PROP_REG           ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB22    ,      ZL303XX_MEM_SIZE_4_BYTE)
#define ZLS3072X_PSL_COE_REG        ZL303XX_MAKE_MEM_ADDR_72X(REG_DCRB17    ,      ZL303XX_MEM_SIZE_4_BYTE)


/* The device ID is actually 2 bytes but the upper byte is not used. */
#define ZL303XX_DEVICE_ID_MASK_72X            (Uint32T)0xF0
#define ZL303XX_DEVICE_ID_SHIFT_72X           (Uint32T)4
#define ZL303XX_DEVICE_REV_MASK_72X           (Uint32T)0x0F
#define ZL303XX_DEVICE_REV_SHIFT_72X          (Uint32T)0

#define ZL303XX_DEVICE_RESET_MASK_72X         (Uint32T)0x80
#define ZL303XX_DEVICE_RESET_SHIFT_72X        (Uint32T)8

#define ZL303XX_DEVICE_INPUT_CLOCK_VALID_MASK_72X     (Uint32T)0x07


/* Generic bit shift */
#define ZL303XX_SHIFT_0 (0)
#define ZL303XX_SHIFT_1 (1)
#define ZL303XX_SHIFT_2 (2)
#define ZL303XX_SHIFT_3 (3)
#define ZL303XX_SHIFT_4 (4)
#define ZL303XX_SHIFT_5 (5)
#define ZL303XX_SHIFT_6 (6)
#define ZL303XX_SHIFT_7 (7)

/* Generic bit mask */
#define ZL303XX_ONE_BITS      (0x1)
#define ZL303XX_TWO_BITS      (0x3)
#define ZL303XX_THREE_BITS    (0x7)
#define ZL303XX_FOUR_BITS     (0xf)
#define ZL303XX_FIVE_BITS     (0x1f)
#define ZL303XX_SIX_BITS      (0x3f)
#define ZL303XX_SEVEN_BITS    (0x7f)
#define ZL303XX_EIGHT_BITS    (0xff)


/* ICxSR resgister bits */
#define ZL303XX_ACVAL_BIT     (4)
#define ZL303XX_PCVAL_BIT     (3)
#define ZL303XX_PPVAL_BIT     (2)

/* MONxCR2 resgister bits */
#define ZL303XX_ACEN_BIT      (2)
#define ZL303XX_PCEN_BIT      (1)
#define ZL303XX_PPEN_BIT      (0)

/* OSRxSR resgister bits */
#define ZL303XX_LSCLKL_BIT    (6)
#define ZL303XX_SRLEN_BIT     (7)

/* DPLLCR1 register bits */
#define ZL303XX_DPLLMODE_BIT  (0)

/* PTAB1 register bits */
#define ZL303XX_SELREF_BIT    (0)

/* VALSR1 register bits */
#define ZL303XX_IC1V_BIT      (0)
#define ZL303XX_IC1L_BIT      (1)
#define ZL303XX_IC2V_BIT      (4)
#define ZL303XX_IC2L_BIT      (5)

/* VALSRZL303XX_2 register bits */
#define ZL303XX_IC3V_BIT      (0)
#define ZL303XX_IC3L_BIT      (1)



#ifdef __cplusplus
}
#endif


#endif /* MULTIPLE INCLUDE BARRIER */

