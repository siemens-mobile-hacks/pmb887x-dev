#pragma once

#include <pmb887x.h>

#define DLL_PUBLIC __attribute__ ((visibility ("default")))

typedef struct I2C_MSG I2C_MSG;

typedef struct I2C_MSG {
  char chip_addr;
#ifdef NEWSGOLD
  char unk1;
  char unk2;
  char unk3;
  short nRegister;
#else
  char  unk1;
  short nRegister;
  char  unk2;
  char  tf; // 1 - tx, 2 - rx
#endif
  short cb_data;
  int (*callback)(I2C_MSG *i2c_msg, int status);
  void *data;
  int size;
} I2C_MSG;

int i2c_transfer(I2C_MSG *msg);
int i2c_receive(I2C_MSG *msg);
