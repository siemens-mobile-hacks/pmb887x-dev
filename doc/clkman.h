#ifndef CLKMAN_H_
#define CLKMAN_H_

#ifdef NEWSGOLD
#define DSP_MASK      (1 << 0)
#define CPU_MASK      (1 << 1)
#define DMA_MASK      (1 << 2)
#define CLKOUT0_MASK  (1 << 4)
#define CLKOUT1_MASK  (1 << 5)
#define CLKOUT2_MASK  (1 << 6)
#define CLK32K_MASK   (1 << 7)
#define FM_MASK       (1 << 8) //на E71 FM Radio,
#define GPTU0_MASK    (1 << 8)
#define CAMIF_MASK    (1 << 12)
#define DIF_MASK      (1 << 13)
#define MMICIF_MASK   (1 << 14)
#define FCDP_MASK     (1 << 15)
#define MMC_MASK      (1 << 16)
#define FIRDA_MASK    (1 << 17)
#define USIF_MASK     (1 << 18)
#define I2C_MASK      (1 << 19)
#define USIM_MASK     (1 << 20)
#define SSC0_MASK     (1 << 21)
#define USART0_MASK   (1 << 22)
#define USART1_MASK   (1 << 23)
#define USB_MASK      (1 << 24)
#define CAPCOM_MASK   (1 << 25)
#define GPTU_MASK     (1 << 26)
#define MEASIF_MASK   (1 << 27)
#define GPRSGEA_MASK  (1 << 28)
#define AFC_MASK      (1 << 29)
#define GSMSYSIF_MASK (1 << 30)
#else//SGOLD
#define CAPCOM_MASK   (1 << 18)
#endif

//cpu client id`s
#ifdef NEWSGOLD
#define  CPU          0x10000
#define  DSP          0x10001  //MODEM
#define  DMA          0x10002
#define  CLKOUT_2     0x10005  //SG2-CAMIF-CLKOUT2
#define  CLK32K       0x10006  //Bluetooth
#define  FM           0x10007
#define  CAMIF        0x10008  //SG2-CAMIF-AHBPER
#define  DIF          0x10009
#define  MMC          0x1000C
#define  I2C          0x1000F
#define  USIM         0x10010
#define  USART0       0x10012
#define  USART1       0x10013
#define  USB          0x10014
#define  MAM          0x100001
// cpu clients: L1CTRL, MODEMAN, FFS, UI-MSOCPU, JAVA, BFC-Test
#endif

#ifdef SGOLD
#define  USART0       0x1003
#define  USART1       0x2003
#define  USB          0x4003
#define  DIF          0x10003
#define  SSC0         0x20003
#define  SIM          0x8003
#define  A2I          0x20003
// dsp clients: L1CTRL, MODEM, FLASH_MGR, MMI
// cpu clients: UI-MSOCPU, FFS, MediaPlayer 
#endif

//??? CPUClkLevels: 0-Off, 1-Lowest(52MHz), 2-Low(52MHz), 3-standart(52MHz), 4-High(104MHz), 5-Highest(208MHz), 6-standart(52MHz)

#ifdef   C81
#define    ClkAllocClient         ((unsigned(*)(int id, const char *name)) 0xA01CFF54)
#define    ClkFreeClient          ((unsigned(*)(unsigned client)) 0xA01D02C0)
#define    ClkSetLevel            ((int(*)(unsigned client,int level) 0xA01D038C)
#define    ClkGetState            ((unsigned(*)(void)) 0xA01D04D8)
#define    ClkStateOn             ((void(*)(unsigned mask)) 0xA01D0444)
#define    ClkStateOff            ((void(*)(unsigned mask)) 0xA01D0468)
#endif

#ifdef   S75v52
#define    ClkAllocClient         ((unsigned(*)(int id, const char *name)) 0xA01D09AC)
#define    ClkFreeClient          ((unsigned(*)(unsigned client)) 0xA01D0D18)
#define    ClkSetLevel            ((int(*)(unsigned client,int level) 0xA01D0DE4)
#define    ClkGetState            ((unsigned(*)(void)) 0xA01D0F30)
#define    ClkStateOn             ((void(*)(unsigned mask)) 0xA01D0E9C)
#define    ClkStateOff            ((void(*)(unsigned mask)) 0xA01D0EC0)
#endif

#ifdef   E71v45
#define    ClkAllocClient         ((unsigned(*)(int id, const char *name)) 0xA04D0260)
#define    ClkFreeClient          ((unsigned(*)(unsigned client)) 0xA04D0658)
#define    ClkSetLevel            ((int(*)(unsigned client,int level))0xA04D0728)
#define    ClkGetState            ((unsigned(*)(void)) 0xA04D0894)
#define    ClkStateOn             ((void(*)(unsigned mask)) 0xA04D07E0)
#define    ClkStateOff            ((void(*)(unsigned mask)) 0xA04D0804)
#endif

#ifdef   EL71v45
#define    ClkAllocClient         ((unsigned(*)(int id, const char *name)) 0xA04D6628)
#define    ClkFreeClient          ((unsigned(*)(unsigned client)) 0xA04D6A20)
#define    ClkSetLevel            ((int(*)(unsigned client,int level))0xA04D0728)
#define    ClkGetState            ((unsigned(*)(void)) 0xA04D6C5C)
#define    ClkStateOn             ((void(*)(unsigned mask)) 0xA04D6BA8)
#define    ClkStateOff            ((void(*)(unsigned mask)) 0xA04D6BCC)
#endif

#ifdef  CX70v56   
#define    ClkAllocCPUClient      ((unsigned(*)(char *name)) 0xA11675F8)  
#define    ClkFreeCPUClient       ((unsigned(*)(unsigned client)) 0xA1167784)
#define    ClkAllocClient         ((unsigned(*)(char *name, int id)) 0xA1167628)
#define    ClkFreeClient          ((unsigned(*)(unsigned client)) 0xA11677B0)
#define    ClkSeLevel             ((int(*)(unsigned client,int level))0xA11677DC)
#define    ClkGetState            ((unsigned(*)(void)) 0xA116797C)
#define    ClkStateOn             ((void(*)(unsigned mask)) 0xA11678FC)
#define    ClkStateOff            ((void(*)(unsigned mask)) 0xA1167920)

#endif

#ifdef  CX75v25
#define    ClkAllocCPUClient      ((unsigned(*)(char *name)) 0xA0A91BF0)
#define    ClkFreeCPUClient       ((unsigned(*)(unsigned client)) A0A91D7C)
#define    ClkAllocClient         ((unsigned(*)(char *name)) 0xA0A91C20)
#define    ClkFreeClient          ((unsigned(*)(unsigned client)) 0xA0A91DA8)
#define    ClkSeLevel             ((int(*)(unsigned client,int level))0xA0A91DD4)
#define    ClkGetState            ((unsigned(*)(void)) 0xA0A91F74)
#define    ClkStateOn             ((void(*)(unsigned mask)) 0xA0A91EF4)
#define    ClkStateOff            ((void(*)(unsigned mask)) 0xA0A91F18)
#endif

#endif /* CLKMAN_H_ */

//“актовый сигнал от радиотракта(26 mHz) идет на: CAPCOM0, CAPCOM1, GPTU0, GPTU1...

/* (c) alfinant@yandex.ru  */
