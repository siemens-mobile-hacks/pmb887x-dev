#pragma once
#include <pmb887x.h>

#define HZ		100

#define writel(v, d)	REG(d) = v
#define readl(d)	REG(d)

#define printk printf

unsigned int __delay(unsigned int loops);
unsigned long calibrate_delay_converge(void);
