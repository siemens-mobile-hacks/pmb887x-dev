#ifndef _REG8876_H_
#define _REG8876_H_

#include <stdint.h>

#define __IO volatile

/******************************************************************************/
/*                                                                            */
/*         Universal Synchronous Asynchronous Receiver Transmitter            */
/*                                                                            */
/******************************************************************************/

typedef struct
{
   __IO uint32_t CLC;                                                      /*!< Clock Control Register */
   __IO uint32_t PISEL;                                                    /*!< Peripheral Input Select Register */
   __IO uint32_t PERID;                                                    /*!< Peripheral ID Number Register  */
   __IO uint32_t RESERVED0;  
   __IO uint32_t CON;                                                      /*!< Control Register */
   __IO uint32_t BG;                                                       /*!< Baudrate Timer Reload Register */
   __IO uint32_t FDV;                                                      /*!< Fractional Divider Register */
   __IO uint32_t PWM;                                                      /*!< IrDA Pulse Mode and Width Register */
   __IO uint32_t TBUF;                                                     /*!< Transmit Buffer Register */
   __IO uint32_t RBUF;                                                     /*!< Receive Buffer Register  */
   __IO uint32_t RESERVED1[2];
   __IO uint32_t ABCON;                                                    /*!< Autobaud Control Register */
   __IO uint32_t ABSTAT;                                                   /*!< Autobaud Status Register */
   __IO uint32_t RESERVED2[2];
   __IO uint32_t RXFCON;                                                   /*!< Receive FIFO Control Register */
   __IO uint32_t TXFCON;                                                   /*!< Transmit FIFO Control Register */
   __IO uint32_t FSTAT;                                                    /*!< FIFO Status Register */
   __IO uint32_t RESERVED3;
   __IO uint32_t WHBCON;			                           /*!< Write Hardware Modified Control Register */
   __IO uint32_t WHBABCON;			                           /*!< Write Hardware Modified Autobaud Control Register */
   __IO uint32_t WHBABSTAT;                                                /*!< Write Hardware Modified Autobaud Status Register */
   __IO uint32_t FCCON;                                                    /*!< FlowControl Register */
   __IO uint32_t FCSTAT;                                                   /*!< FlowControl Status Register */
   __IO uint32_t ICON;                                                     /*!< Interrupt Control Register */
   __IO uint32_t ISTAT;                                                    /*!< Interrupt Status Register */
   __IO uint32_t UNK0;                                                     
   __IO uint32_t IFLCLR;                                                   /*!< Interrupt Flag Clear Register */
   __IO uint32_t IFLSET;                                                   /*!< Interrupt Flag Set Register */
   __IO uint32_t UNK1;                                                    
   __IO uint32_t TMO;                                                      /*!< RX Timeout Control Register */

} USART_TypeDef; 

/*!< Peripheral memory map */

#define USART0_BASE                     ((uint32_t)0xF1000000)
#define USART1_BASE                     ((uint32_t)0xF1800000)

/*!< Peripheral_declaration */  

__no_init USART_TypeDef S0  @ USART0_BASE;     
__no_init USART_TypeDef S1  @ USART1_BASE; 

/*************************** USART Interrupt Numbers **************************/

#define USART0_TX_IRQn                0x04
#define USART0_TBUF_IRQn              0x05
#define USART0_RX_IRQn                0x06
#define USART0_ERR_IRQn               0x07
#define USART0_CTS_IRQn               0x08
#define USART0_ABDET_IRQn             0x09
#define USART0_ABSTART_IRQn           0x0A
#define USART0_TMO_IRQn               0x0B

/********************* Bit Mask for Interrupt Control *******************/

#define USART_IC_TMO                  (1 <<  7)                                 /*!< RX Timeout Interrupt Mask */
#define USART_IC_CTS                  (1 <<  6)                                 /*!< CTS Interrupt Mask */
#define USART_IC_ABDET                (1 <<  5)                                 /*!< Autobaud Detected Interrupt Mask */
#define USART_IC_ABSTART              (1 <<  4)                                 /*!< Autobaud Start Interrupt Mask*/
#define USART_IC_ERR                  (1 <<  3)                                 /*!< Error Interrupt Mask */
#define USART_IC_RX                   (1 <<  2)                                 /*!< Receive Interrupt Mask */
#define USART_IC_TBUF                 (1 <<  1)                                 /*!< Transmit Buffer Interrupt Mask */
#define USART_IC_TX                   (1 <<  0)                                 /*!< Transmit Interrupt Mask */

/******************************************************************************/ 
/*                                                                            */
/*                                 GPIO                                       */
/*                                                                            */
/******************************************************************************/

typedef struct
{
   __IO uint32_t CLC;                    //0xF4300000 
   __IO uint32_t RESERVED0;
   __IO uint32_t ID;                     //0xF4300008
   __IO uint32_t RESERVED1;
   __IO uint32_t UNK10[4];   
   __IO uint32_t KP_IN0;                 //0xF4300020  
   __IO uint32_t KP_IN1;                 //0xF4300024 
   __IO uint32_t KP_IN2;                 //0xF4300028 
   __IO uint32_t KP_IN3;                 //0xF430002C 
   __IO uint32_t KP_IN4;                 //0xF4300030 
   __IO uint32_t KP_IN5;                 //0xF4300034
   __IO uint32_t KP_OUT5_OUT6;           //0xF4300038 
   __IO uint32_t KP_OUT0;                //0xF430003C         /* '1', '4', '7', '*' */
   __IO uint32_t KP_OUT1;                //0xF4300040         /* '2', '5', '8'', '0' */
   __IO uint32_t KP_OUT2;                //0xF4300044         /* '3', '6', '9', '#' */
   __IO uint32_t KP_OUT3;                //0xF4300048 
   __IO uint32_t USART0_RXD;             //0xF430004C         /*-->AC_RX, 4й пин на коннекторе Nano, Func4(GPTU1.??) */
   __IO uint32_t USART0_TXD;             //0xF4300050         /*-->AC_TX, 3й пин на коннекторе Nano, +подт€гивающий резистор, Func3(CCU0.CC2) */
   __IO uint32_t USART0_CTS;             //0xF4300054         /*-->AC_CTS, 2й пин на коннекторе Nano, +подт€гивающий резистор, Func3(CCU0.CC6) */   
   __IO uint32_t USART0_RTS;             //0xF4300058         /*-->AC_RTS, 1й пин на коннекторе Nano, +подт€гивающий резистор, Func3(CCU1.CC2) */
   __IO uint32_t DSPOUT0;                //0xF430005C         /*-->AC_DCD, 6й пин на коннекторе Nano, +подт€гивающий резистор, Func3(CCU1.CC6) */  
   __IO uint32_t USART1_RXD;             //0xF4300060         /*--> BT_RX */
   __IO uint32_t USART1_TXD;             //0xF4300064         /*-->  */    
   __IO uint32_t USART1_RTS;             //0xF4300068         /*-->BT_RTS */
   __IO uint32_t USART1_CTS;             //0xF430006C         /*-->BT_CTS */   
   __IO uint32_t USB_DPLUS;              //0xF4300070    
   __IO uint32_t USB_DMINUS;             //0xF4300074 
   __IO uint32_t UNK78;                  //0xF4300078         интерфейс диспле€?
   __IO uint32_t UNK7C;                  //0xF430007C
   __IO uint32_t UNK80;                  //0xF4300080
   __IO uint32_t UNK84;                  //0xF4300084
   __IO uint32_t UNK88;                  //0xF4300088
   __IO uint32_t UNK8C;                  //0xF430008C   
   __IO uint32_t I2C_SCL;                //0xF4300090         /*-->I2C_CLK */
   __IO uint32_t I2C_SDA;                //0xF4300094         /*-->I2C_DAT */
   __IO uint32_t UNK98;                  //0xF4300098         /* начинает глючить дисплей */
   __IO uint32_t I2S_UNK0;               //0xF430009C         I2S2, отвечают за звук 
   __IO uint32_t I2S_UNk1;               //0xF43000A0
   __IO uint32_t I2S_UNk2;               //0xF43000A4
   __IO uint32_t I2S_UNk3;               //0xF43000A8
   __IO uint32_t UNKAC;                  //0xF43000AC
   __IO uint32_t UNKB0;                  //0xF43000B0
   __IO uint32_t UNKB4;                  //0xF43000B4
   __IO uint32_t UNKB8;                  //0xF43000B8
   __IO uint32_t UNKBC;                  //0xF43000BC
   __IO uint32_t UNKC0;                  //0xF43000C0
   __IO uint32_t UNKC4;                  //0xF43000C4
   __IO uint32_t UNKC8;                  //0xF43000C8   
   __IO uint32_t TOUT0;                  //0xF43000CC         /*-->LED_FL_OFF */
   __IO uint32_t TOUT1;                  //0xF43000D0         /*-->PM_CHARGE_UC, вкл/выкл зар€дку аккумул€тора */
   __IO uint32_t UNKD4;                  //0xF43000D4
   __IO uint32_t UNKD8;                  //0xF43000D8
   __IO uint32_t UNKDC;                  //0xF43000DC
   __IO uint32_t UNKE0;                  //0xF43000E0
   __IO uint32_t UNKE4;                  //0xF43000E4      
   __IO uint32_t TOUT7;                  //0xF43000E8         /*-->PM_RF2_EN, отвечает за питание радиотракта */ 
   __IO uint32_t UNKEC;                  //0xF43000EC    
   __IO uint32_t TOUT9;                  //0xF43000F0         /*-->I2C_2_DAT */
   __IO uint32_t TOUT10;                 //0xF43000F4         /*-->SERIAL_EN */  
   __IO uint32_t TOUT11;                 //0xF43000F8         /*-->I2C_2_CLK */
   __IO uint32_t UNKFC;                  //0xF43000FC 
   __IO uint32_t RF_STR0;                //0xF4300100         /*-->RF_STR */
   __IO uint32_t RF_STR1;                //0xF4300104         /*-->PM_RINGIN, идет на Dialog,используетс€ дл€ генерации звуков посредством Ў»ћ(CAPCOM) */
   __IO uint32_t RF_DATA;                //0xF4300108         /*?-->RF_DAT */
   __IO uint32_t RF_CLK;                 //0xF430010C         /*?-->RF_CLK */
   __IO uint32_t UNK110;                 //0xF4300110   
   __IO uint32_t UNK114;                 //0xF4300114         /* тер€етс€ св€зь по блютузу */ 
   __IO uint32_t DSPOUT1;                //0xF4300118         /*-->PM_WADOG, собака cидит в PMU(Dialog/Twigo) */
   __IO uint32_t DSPIN1;                 //0xF430011C         /*-->LED_FL_EN */
   __IO uint32_t UNK120;                 //0xF4300120
   __IO uint32_t UNK124;                 //0xF4300124
   __IO uint32_t UNK128;                 //0xF4300128
   __IO uint32_t UNK12C;                 //0xF430012C
   __IO uint32_t UNK130;                 //0xF4300130
   __IO uint32_t UNK134;                 //0xF4300134
   __IO uint32_t UNK138;                 //0xF4300138
   __IO uint32_t UNK13C;                 //0xF430013C
   __IO uint32_t UNK140;                 //0xF4300140
   __IO uint32_t UNK144;                 //0xF4300144
   __IO uint32_t UNK148;                 //0xF4300148
   __IO uint32_t UNK14C;                 //0xF430014C
   __IO uint32_t UNK150;                 //0xF4300150
   __IO uint32_t UNK154;                 //0xF4300154   
   __IO uint32_t CIF_D0;                 //0xF4300158         /*-->CAM_DAT0, на камеру */
   __IO uint32_t CIF_D1;                 //0xF430015C         /*-->CAM_DAT1, на камеру */
   __IO uint32_t CIF_D2;                 //0xF4300160         /*-->CAM_DAT2, на камеру */
   __IO uint32_t CIF_D3;                 //0xF4300164         /*-->CAM_DAT3, на камеру */
   __IO uint32_t CIF_D4;                 //0xF4300168         /*-->CAM_DAT4, на камеру */
   __IO uint32_t CIF_D5;                 //0xF430016C         /*-->CAM_DAT5, на камеру */
   __IO uint32_t CIF_D6;                 //0xF4300170         /*-->CAM_DAT6, на камеру */
   __IO uint32_t CIF_D7;                 //0xF4300174         /*-->CAM_DAT7, на камеру */
   __IO uint32_t CIF_PCLK;               //0xF4300178         /*-->CAM_PCLK, на камеру */
   __IO uint32_t CIF_HSYNC;              //0xF430017C         /*-->CAM_HSYNC, на камеру */
   __IO uint32_t CIF_VSYNC;              //0xF4300180         /*-->CAM_VSYNC, на камеру */
   __IO uint32_t CLKOUT2;                //0xF4300184         /*-->CAM_CLKOUT, на камеру */
   __IO uint32_t UNK188;                 //0xF4300188
   __IO uint32_t UNK18C;                 //0xF430018C   
   __IO uint32_t UNK190;                 //0xF4300190
   __IO uint32_t UNK194;                 //0xF4300194
   __IO uint32_t UNK198;                 //0xF4300198
   __IO uint32_t UNK19C;                 //0xF430019C   
   __IO uint32_t UNK1A0;                 //0xF43001A0
   __IO uint32_t UNK1A4;                 //0xF43001A4
   __IO uint32_t UNK1A8;                 //0xF43001A8
   __IO uint32_t UNK1AC;                 //0xF43001AC
   __IO uint32_t UNK1B0;                 //0xF43001B0
   __IO uint32_t UNK1B4;                 //0xF43001B4
   __IO uint32_t UNK1B8;                 //0xF43001B8
   __IO uint32_t UNK1BC;                 //0xF43001BC  
   __IO uint32_t MMCI_UNK0;              //0xF43001C0         /* интерфейс карты пам€ти */
   __IO uint32_t MMCI_UNK1;              //0xF43001C4
   __IO uint32_t MMCI_UNK2;              //0xF43001C8
   __IO uint32_t UNK1CC;                 //0xF43001CC
   __IO uint32_t UNK1D0;                 //0xF43001D0 
   __IO uint32_t UNK1D4;                 //0xF43001D4 
   __IO uint32_t UNK1D8;                 //0xF43001D8 
   __IO uint32_t UNK1DC;                 //0xF43001DC    
   __IO uint32_t I2S1_CLK1;              //0xF43001E0         /*--> LED_FL_TRIG */
   __IO uint32_t CIF_PD;                 //0xF43001E4         /* ¬кл/выкл питание камеры (DVDD_CAMERA=2,88 volt) */
     
} GPIO_TypeDef; 

/*!< Peripheral memory map */

#define GPIO_BASE                     ((uint32_t)0xF4300000)

/*!< Peripheral_declaration */  

__no_init GPIO_TypeDef GPIO  @ GPIO_BASE;     

/**********************  Bit Mask for GPIO register  *************************/ 

#define  GPIO_IS                      (7 << 0)                                  /*! [2:0] (Input Selection) «начение битов определ€ют какой интерфейс получает вход€щие данные,если PS=0 */
#define  GPIO_OS                      (7 << 4)                                  /*! [6:4] (Output Selection) «начение битов определ€ют какой интерфейс управл€ет выходом,если PS=0 */
#define  GPIO_PS                      (1 << 8)                                  /*! [8] (Port Selection) ”правление : 0-вывод управл€етс€ каким-то интерфейсом(зависит от конфигурации IS и OS полей),1-ћы управл€ем:) */
#define  GPIO_DAT                     (1 << 9)                                  /*! [9] (Data of GPIOx) –егистр данных,определ€ет значение напр€жени€ на выводе: 0-низкий уровень, 1-высокий уровень */
#define  GPIO_DIR                     (1 << 10)                                 /*! [10] (Direction Control) Ќаправление передачи данных: 0-вывод настроен вход, 1-вывод настроен ны выход */
#define  GPIO_PPEN                    (1 << 12)                                 /*! [12] (Push/Pull Enable), 0-Push/pull(—тандартный выход), 1-Open drain(открытый сток) */
#define  GPIO_PDPU                    (3 << 13)                                 /*! [14:13] (Pullup/Pulldown Selection) ¬кл/¬ыкл внутреннего подт€гивающего резистра: 00-без подт€жки ,01-Pullup(подт€гивающий резистор подключен к плюсу), 02-Pulldown(подт€гивающий резистор подключен к земле) */                     
#define  GPIO_ENAQ                    (1 << 15)                                 /*! [15] 0-—осто€ние вывода определ€етс€ другими битами, 1-¬ывод сброшен, высокоомное состо€ние */

/**************************  Definition for PDPU ******************************/ 

#define  PULLUP                      1
#define  PULLDOWN                    2

/**************************  Definition for PPEN ******************************/

#define  PUSHPULL                    0
#define  OPENDRAIN                   1

//IRQ_0x5C_GPTU0   0xA04D12D4 , E71
//IRQ_0x5D_GPTU0   0xA04D12D4 , E71    


/******************************************************************************/ 
/*                                                                            */
/*                                 CAPCOM                                     */
/*                                                                            */
/******************************************************************************/

#define   capcom_hw_clk                 26000000                /* 26 MHz */

typedef struct
{
   __IO uint32_t CLC;
   __IO uint32_t CCPISEL;              /* Port Input Selection Register */
   __IO uint32_t CCID;                 /* CAPCOM Identification Register */
   __IO uint32_t ZERO1;  
   __IO uint32_t T01CON;               /* Timer Control Registers */
   __IO uint32_t CCM0;                 /* Capture/Compare Mode Registers (CC0...CC3) */
   __IO uint32_t CCM1;                 /* Capture/Compare Mode Registers (CC4...CC7) */
   uint32_t RESERVED1[2];   
   __IO uint32_t CCOUT;                /* Compare Output Registers */
   __IO uint32_t CCIOC;                /* I/O Control Register */
   __IO uint32_t CCSEE;                /* Single Event Enable Registers */
   __IO uint32_t CCSEM;                /* Single Event Mode Registers */
   __IO uint32_t CCDRM;                /* Double-Register Compare Mode Register */
   uint32_t  RESERVED3[2];
   __IO uint32_t T0;                   /* Timer 0 register that store timer value */
   __IO uint32_t T0REL;                /* Timer 0 reload register that store the timer reload value */
   __IO uint32_t T1;                   /* Timer 1 register that store timer value */
   __IO uint32_t T1REL;                /* Timer 1 reload register that store the timer reload value */
   __IO uint32_t CC0;                  /* Data registers for the capture or compare operations */
   __IO uint32_t CC1; 
   __IO uint32_t CC2;
   __IO uint32_t CC3;
   __IO uint32_t CC4;
   __IO uint32_t CC5;
   __IO uint32_t CC6;
   __IO uint32_t CC7;  
   uint32_t  RESERVED5[8];
   __IO uint32_t ZERO2; 
   __IO uint32_t ZERO3;  
   __IO uint32_t ZERO4;
   __IO uint32_t ZERO5;
   uint32_t  RESERVED13[14];
   __IO uint32_t CC7IC;                /* Interrupt control register for CC7 */
   __IO uint32_t CC6IC;                /* Interrupt control register for CC6 */
   __IO uint32_t CC5IC;                /* Interrupt control register for CC5 */
   __IO uint32_t CC4IC;                /* Interrupt control register for CC4 */
   __IO uint32_t CC3IC;                /* Interrupt control register for CC3 */
   __IO uint32_t CC2IC;                /* Interrupt control register for CC2 */
   __IO uint32_t CC1IC;                /* Interrupt control register for CC1 */
   __IO uint32_t CC0IC;                /* Interrupt control register for CC0 */
   __IO uint32_t T1IC;                 /* Interrupt is requested on overflow of Timer 1 */
   __IO uint32_t T0IC;                 /* Interrupt is requested on overflow of Timer 0 */
   
} CAPCOM_TypeDef;  

/*!< Peripheral memory map */

#define CAPCOM0_BASE                     ((uint32_t)0xF4000000)
#define CAPCOM1_BASE                     ((uint32_t)0xF4100000)

/*!< Peripheral_declaration */  

__no_init CAPCOM_TypeDef CCU0  @ CAPCOM0_BASE; 
__no_init CAPCOM_TypeDef CCU1  @ CAPCOM1_BASE; 

/*************************** CAPCOM Interrupt Numbers ********************************/

#define CCU0_T0_IRQn             0x48
#define CCU0_T1_IRQn             0x49
#define CCU0_CC0_IRQn            0x4A
#define CCU0_CC1_IRQn            0x4B
#define CCU0_CC2_IRQn            0x4C
#define CCU0_CC3_IRQn            0x4D
#define CCU0_CC4_IRQn            0x4E
#define CCU0_CC5_IRQn            0x4F
#define CCU0_CC6_IRQn            0x50
#define CCU0_CC7_IRQn            0x51

#define CCU1_T0_IRQn             0x52
#define CCU1_T1_IRQn             0x53
#define CCU1_CC0_IRQn            0x54
#define CCU1_CC1_IRQn            0x55
#define CCU1_CC2_IRQn            0x56
#define CCU1_CC3_IRQn            0x57
#define CCU1_CC4_IRQn            0x58
#define CCU1_CC5_IRQn            0x59
#define CCU1_CC6_IRQn            0x5A
#define CCU1_CC7_IRQn            0x5B

/*******************  Bit Mask for  for CCPISEL register  ********************/ 

#define  C1C0IS                          0                                      /*!< Select Source for Capture Input Channels C0,C1 (0-Default input port,1-Alternate input port) */
#define  C3C2IS                          1                                      /*!< Select Source for Capture Input Channels C2,C3 (0-Default input port,1-Alternate input port) */
#define  C5C4IS                          2                                      /*!< Select Source for Capture Input Channels C4,C5 (0-Default input port,1-Alternate input port) */
#define  C7C6IS                          3                                      /*!< Select Source for Capture Input Channels C6,C7 (0-Default input port,1-Alternate input port) */
#define  T0INIS                          5                                      /*!< Select Source for External Input of Timer 0 Clock (0-pin selected (default),1-no input received) */
#define  T1INIS                          6                                      /*!< Select Source for External Input of Timer 1 Clock (0-signal from GSM Timer??,1-no input received) */

/*******************  Bit Mask for  for T01CON register  **********************/ 

#define  T0I                          (7 << 0)                                  /*!< [2:0] (Timer/Counter 0 Input Selection) */                     /*!< Bit 2 */
#define  T0M                          (1 << 3)                                  /*!< [3] Timer/Counter 0 Mode Selection (0-Timer Mode,1-Counter Mode) */
#define  T0R                          (1 << 6)                                  /*!< [6] Timer/Counter 0 Run Control (0-disabled,1-enabled) */

#define  T1I                          (7 << 8)                                  /*!< [10:8] Timer/Counter 1 Input Selection) */
#define  T1M                          (1 << 11)                                 /*!< [11] Timer/Counter 1 Mode Selection (0-Timer Mode,1-Counter Mode) */
#define  T1R                          (1 << 14)                                 /*!< [14] Timer/Counter 1 Run Control (0-disabled,1-enabled) */

/*********************  Bit Mask for  for CCM0 register  *********************/ 

#define  CCM0_MOD0                     (7 << 0)                                 /*!< [2:0] bits (Capture/Compare mode selection for CC0) */
#define  CCM0_ACC0                     (1 << 3)                                 /*!< [3] Select allocated Timer (0-timer T0 ,1-timer T1) */
#define  CCM0_MOD1                     (7 << 4)                                 /*!< [6:4] bits (Capture/Compare mode selection for CC1)*/
#define  CCM0_ACC1                     (1 << 7)                                 /*!< [7] Select allocated Timer (0-timer T0 ,1-timer T1) */
#define  CCM0_MOD2                     (7 << 8)                                 /*!< [10:8] bits (Capture/Compare mode selection for CC2) */ 
#define  CCM0_ACC2                     (1 << 11)                                /*!< [11] Select allocated Timer (0-timer T0 ,1-timer T1) */
#define  CCM0_MOD3                     (7 << 12)                                /*!< [14:12] bits (Capture/Compare mode selection for CC3) */
#define  CCM0_ACC3                     (1 << 15)                                     /*!< [15] Select allocated Timer (0-timer T0 ,1-timer T1) */

/********************  Bit Mask for  for CCM1 register  **********************/ 

#define  CCM1_MOD4                     (7 << 0)                                 /*!< [2:0] bits (Capture/Compare mode selection for CC4) */
#define  CCM1_ACC4                     (1 << 3)                                 /*!< [3] Select allocated Timer (0-timer T0 ,1-timer T1) */
#define  CCM1_MOD5                     (7 << 4)                                 /*!< [6:4] bits (Capture/Compare mode selection for CC5)*/
#define  CCM1_ACC5                     (1 << 7)                                 /*!< [7] Select allocated Timer (0-timer T0 ,1-timer T1) */
#define  CCM1_MOD6                     (7 << 8)                                 /*!< [10:8] bits (Capture/Compare mode selection for CC6) */ 
#define  CCM1_ACC6                     (1 << 11)                                /*!< [11] Select allocated Timer (0-timer T0 ,1-timer T1) */
#define  CCM1_MOD7                     (7 << 12)                                /*!< [14:12] bits (Capture/Compare mode selection for CC7) */
#define  CCM1_ACC7                     (1 << 15)                                /*!< [15] Select allocated Timer (0-timer T0 ,1-timer T1) */

/*********************  Definitions  for CCMx Registers *********************/ 

#define  MODE_NONE                       0                                      /*!< Disable Capture and Compare Modes. The respective CAPCOM registers may be used for general variable storage */
#define  MODE_CAPTURE_0                  1                                      /*!< Capture on Positive Transition (Rising Edge) at Pin CCxIO */
#define  MODE_CAPTURE_1                  2                                      /*!< Capture on Negative Transition (Falling Edge) at Pin CCxIO */
#define  MODE_CAPTURE_2                  3                                      /*!< Capture on Positive and Negative Transition (Both Edges) at Pin CCxIO*/
#define  MODE_COMPARE_0                  4                                      /*!< Compare Mode 0: Interrupt Only Several interrupts per timer period; Enables double-register compare mode for registers CC4...CC7 */
#define  MODE_COMPARE_1                  5                                      /*!< Compare Mode 1: Toggle Output Pin on each Match. Several compare events per timer period; This mode is required for double- register compare mode for registers CC0...CC3.*/
#define  MODE_COMPARE_2                  6                                      /*!< Compare Mode 2: Interrupt Only Only one interrupt per timer period*/
#define  MODE_COMPARE_3                  7                                      /*!< Compare Mode 3: Set Output Pin on each Match. Reset output pin on each timer overflow; Only one interrupt per timer period */                                

/********************  Bit Mask for  for CCOUT register  **********************/ 
                                                                                /* просто показывает какой канал активен */
#define  CCOUT_CC0                       (1 << 0)                               /*!< Compare Output for Channel 0 ) */
#define  CCOUT_CC1                       (1 << 1)                               /*!< Compare Output for Channel 1 ) */
#define  CCOUT_CC2                       (1 << 2)                               /*!< Compare Output for Channel 2 ) */
#define  CCOUT_CC3                       (1 << 3)                               /*!< Compare Output for Channel 4 ) */
#define  CCOUT_CC4                       (1 << 4)                               /*!< Compare Output for Channel 5 ) */
#define  CCOUT_CC5                       (1 << 5)                               /*!< Compare Output for Channel 6 ) */
#define  CCOUT_CC6                       (1 << 6)                               /*!< Compare Output for Channel 7 ) */
#define  CCOUT_CC7                       (1 << 7)                               /*!< Compare Output for Channel 8 ) */

/********************  Bit definition for CCIOC register  **********************/

#define  ORSEL                           (1 << 0)                               /*!< Output Register Select (read only) */
#define  PL                              (1 << 1)                               /*!< Port Lock (0-The contents of the port register is changed by the CAPCOM unit (default),1-The contents of the port register is not changed by the CAPCOM unit) */
#define  STAG                            (1 << 2)                               /*!< Staggered mode (0-enabled(default), 1-disabled) */
#define  PDS                             (1 << 3)                               /*!< Port Direction Select (0-output(default), 1-input) */

/********************  Bit numeration for CCSEE register  ********8*************/ 

#define  CCSEE0                          (1 << 0)                               /*!< Single Event Enable for Channel 0 */
#define  CCSEE1                          (1 << 1)                               /*!< Single Event Enable for Channel 1 */
#define  CCSEE2                          (1 << 2)                               /*!< Single Event Enable for Channel 2 */
#define  CCSEE3                          (1 << 3)                               /*!< Single Event Enable for Channel 3 */
#define  CCSEE4                          (1 << 4)                               /*!< Single Event Enable for Channel 4 */
#define  CCSEE5                          (1 << 5)                               /*!< Single Event Enable for Channel 5 */
#define  CCSEE6                          (1 << 6)                               /*!< Single Event Enable for Channel 6 */
#define  CCSEE7                          (1 << 7)                               /*!< Single Event Enable for Channel 7 */

/********************  Bit numeration for CCSEM register  **********************/ 

#define  CCSEM0                          (1 << 0)                               /*!< Single Event Mode enable for Channel 0 */
#define  CCSEM1                          (1 << 1)                               /*!< Single Event Mode enable for Channel 1 */
#define  CCSEM2                          (1 << 2)                               /*!< Single Event Mode enable for Channel 2 */
#define  CCSEM3                          (1 << 3)                               /*!< Single Event Mode enable for Channel 3 */
#define  CCSEM4                          (1 << 4)                               /*!< Single Event Mode enable for Channel 4 */
#define  CCSEM5                          (1 << 5)                               /*!< Single Event Mode enable for Channel 5 */
#define  CCSEM6                          (1 << 6)                               /*!< Single Event Mode enable for Channel 6 */
#define  CCSEM7                          (1 << 7)                               /*!< Single Event Mode enable for Channel 7 */

/*******************  Bit numeration for CCDxM register  **********************/ 

#define  DR0M                            (3 << 0)                               /*!< [1:0] bits (Double Register 0 compare Mode selection */
#define  DR1M                            (3 << 2)                               /*!< [3:2] bits (Double Register 1 compare Mode selection */
#define  DR2M                            (3 << 4)                               /*!< [5:4] bits (Double Register 2 compare Mode selection */
#define  DR3M                            (3 << 6)                               /*!< [7:6] bits (Double Register 3 compare Mode selection */


/*************  Bit Mask for Interrupt Control Registers  ***************/

#define ICR_IEN                          (1 << 12)                              /* Interrupt Enable Register */
#define ICR_STAT                         (1 << 13)                              /* Interrupt Status Register */
#define ICR_CLRFL                        (1 << 14)                              /* Interrupt Clear Flag Register */
#define ICR_SETFL                        (1 << 15)                              /* Interrupt Set Flag Register */
                                                              
/******************************************************************************/ 
/*                                                                            */
/*                            SCU (System Control Unit)                       */
/*                                                                            */
/******************************************************************************/

#define  RST_CTRL_ST          *( __IO uint32_t*)        0xF4400018          /*!< Reset Control And Status Register */
#define  RTCIF                *( __IO uint32_t*)        0xF4400064          /*!< Real Time Clock Interface Enable */
//#define  ARSTDIS            *( __IO uint32_t*)        0xF44000??          /*!< Application Reset Disable Register */

/******************************************************************************/ 
/*                                                                            */
/*                            STM (System Timer)                              */
/*                                                                            */
/******************************************************************************/

#define  STM_TIM0                     *( __IO uint32_t*)     0xF4B00010 
#define  STM_TIM1                     *( __IO uint32_t*)     0xF4B00014
#define  STM_TIM2                     *( __IO uint32_t*)     0xF4B00018
#define  STM_TIM3                     *( __IO uint32_t*)     0xF4B0001C
#define  STM_TIM4                     *( __IO uint32_t*)     0xF4B00020
#define  STM_TIM5                     *( __IO uint32_t*)     0xF4B00014
#define  STM_TIM6                     *( __IO uint32_t*)     0xF4B00018
#define  STM_TIM7                     *( __IO uint32_t*)     0xF4B0002C


#endif /* _REG8876_H_ */

/* (c) alfinant@yandex.ru  */
