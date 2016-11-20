#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>

#define  __IO    volatile

#ifdef NEWSGOLD

#define  GPIO_KP_IN0                  *( __IO uint32_t*)     0xF4300020 
#define  GPIO_KP_IN1                  *( __IO uint32_t*)     0xF4300024 
#define  GPIO_KP_IN2                  *( __IO uint32_t*)     0xF4300028 
#define  GPIO_KP_IN3                  *( __IO uint32_t*)     0xF430002C 
#define  GPIO_KP_IN4                  *( __IO uint32_t*)     0xF4300030 
#define  GPIO_KP_IN5                  *( __IO uint32_t*)     0xF4300034
#define  GPIO_KP_OUT5_OUT6            *( __IO uint32_t*)     0xF4300038 
#define  GPIO_KP_OUT0                 *( __IO uint32_t*)     0xF430003C 
#define  GPIO_KP_OUT1                 *( __IO uint32_t*)     0xF4300040 
#define  GPIO_KP_OUT2                 *( __IO uint32_t*)     0xF4300044 
#define  GPIO_KP_OUT3                 *( __IO uint32_t*)     0xF4300048 


#define  GPIO_USART0_RXD              *( __IO uint32_t*)     0xF430004C         /* 4й пин на коннекторе елки */
#define  GPIO_USART0_TXD              *( __IO uint32_t*)     0xF4300050         /* 3й пин на коннекторе елки */
#define  GPIO_USART0_RTS              *( __IO uint32_t*)     0xF4300054         /* 2й пин на коннекторе елки */
#define  GPIO_USART0_CTS              *( __IO uint32_t*)     0xF4300058         /* 1й пин на коннекторе елки ,подтянут на + */
#define  GPIO_DSPOUT0                 *( __IO uint32_t*)     0xF430005C         /*-->AC_DCD , 6й пин на коннекторе елки */ 

#define  GPIO_I2C_SCL                 *( __IO uint32_t*)     0xF4300090         /*-->I2C_CLK */
#define  GPIO_I2C_SDA                 *( __IO uint32_t*)     0xF4300094         /*-->I2C_DAT */


#define  GPIO_TOUT1                   *( __IO uint32_t*)     0xF43000D0         /*-->PM_CHARGE_UC, вкл/выкл зарядку аккумулятора */ 
#define  GPIO_TOUT7                   *( __IO uint32_t*)     0xF43000E8         /*-->PM_RF2_EN, отвечает за питание радиотракта */ 
#define  GPIO_TOUT9                   *( __IO uint32_t*)     0xF43000F0         /*-->I2C_2_DAT */
#define  GPIO_TOUT10                  *( __IO uint32_t*)     0xF43000F4         /*-->SERIAL_EN, режим UART, иначе USB */  
#define  GPIO_TOUT11                  *( __IO uint32_t*)     0xF43000F8         /*-->I2C_2_CLK */

/*
#define  0xF430009B
....................... I2S2 ,отвечают за звук 
#define  0xF43000A8
*/

#define  GPIO_RF_STR1                 *( __IO uint32_t*)     0xF4300104         /*! Идет на Dialog(PM_RINGIN),используется для генерации звуков посредством ШИМ */
#define  GPIO_DSPOUT1                 *( __IO uint32_t*)     0xF4300118         /*!-->PM_WADOG, Осторожно!Злая Собака!Надо постоянно дергать его за лапу.Сидит в Dialog/Twigo */

#define  GPIO_CIF_D0                  *( __IO uint32_t*)     0xF4300158         /*!-->CAM_DAT0, на камеру */
#define  GPIO_CIF_D1                  *( __IO uint32_t*)     0xF430015C         /*!-->CAM_DAT1, на камеру */
#define  GPIO_CIF_D2                  *( __IO uint32_t*)     0xF4300160         /*!-->CAM_DAT2, на камеру */
#define  GPIO_CIF_D3                  *( __IO uint32_t*)     0xF4300164         /*!-->CAM_DAT3, на камеру */
#define  GPIO_CIF_D4                  *( __IO uint32_t*)     0xF4300168         /*!-->CAM_DAT4, на камеру */
#define  GPIO_CIF_D5                  *( __IO uint32_t*)     0xF430016C         /*!-->CAM_DAT5, на камеру */
#define  GPIO_CIF_D6                  *( __IO uint32_t*)     0xF4300170         /*!-->CAM_DAT6, на камеру */
#define  GPIO_CIF_D7                  *( __IO uint32_t*)     0xF4300174         /*!-->CAM_DAT7, на камеру */
#define  GPIO_CIF_PCLK                *( __IO uint32_t*)     0xF4300178         /*!-->CAM_PCLK, на камеру */
#define  GPIO_CIF_HSYNC               *( __IO uint32_t*)     0xF430017C         /*!-->CAM_HSYNC, на камеру */
#define  GPIO_CIF_VSYNC               *( __IO uint32_t*)     0xF4300180         /*!-->CAM_VSYNC, на камеру */
#define  GPIO_CLKOUT2                 *( __IO uint32_t*)     0xF4300184         /*!-->CAM_CLKOUT, на камеру */
#define  GPIO_CIF_PD                  *( __IO uint32_t*)     0xF43001E4         /*! Вкл/выкл питание камеры (DVDD_CAMERA=2,88 volt) */
#endif

#ifdef SGOLD
#define  GPIO_TOUT7                   *( __IO uint32_t*)     0xF43000EC 
#endif

/*******************  Bit numeration for GPIO registers  **********************/ 

#define  IS                          0                                          /*! [3:0] (Input Selection) Значение битов определяют какой интерфейс получает входящие данные,если PS=0 */
#define  OS                          4                                          /*! [6:4] (Output Selection) Значение битов определяют какой интерфейс управляет выходом,если PS=0 */
#define  PS                          8                                          /*! [8] (Port Selection) Управление : 0-вывод управляется каким-то интерфейсом(зависит от конфигурации IS и OS полей),1-Мы управляем:) */
#define  DATA                        9                                          /*! [9] (Data of GPIOx) Регистр данных,определяет значение напряжения на выводе: 0-низкий уровень, 1-высокий уровень */
#define  DIR                         10                                         /*! [10] (Direction Control) Направление передачи данных: 0-вывод является входом, 1-вывод является выходом */
#define  PPEN                        12                                         /*! [12] (Push/Pull Enable), 0-Push/pull(Стандартный выход), 1-Open drain(открытый сток) */
#define  PDPU                        13                                         /*! [14:13] (Pullup/Pulldown Selection),00-без подтягивающих резистров,01-Pullup(подтягивающий резистор подключен к плюсу), 02-Pulldown(подтягивающий резистор подключен к земле) */                     
#define  ENAQ                        15                                         /*! [15] 0-Состояние вывода определяется другими битами, 1-Вывод сброшен,низкий уровень.ВНИМАНИЕ!Этот бит влияет только на выход,на входyю функцию не влияет! */

/************************* Definition for IS and OS ***************************/ 
                                                                                
#define  ALT0                        1                                        
#define  ALT1                        2
#define  ALT2                        3
#define  ALT3                        4    
#define  ALT4                        5             
#define  ALT5                        6             
#define  ALT6                        7

/**************************  Definition for PDPU ******************************/ 

#define  PULLUP                      1
#define  PULLDOWN                    2

/**************************  Definition for PPEN ******************************/

#define  PUSHPULL                    0
#define  OPENDRAIN                   1

#endif /* GPIO_H_ */

/* (c) alpha2809  */