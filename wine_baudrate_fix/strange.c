#define __GNU_SOURCE 1

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <dlfcn.h>
#include <asm/termbits.h>
#include <linux/serial.h>
#include <asm/ioctls.h>

static int speed_hack = 0;
static int enable_speed_hack = 0;

int ioctl(int fd, unsigned long request, ...) {
	static int (*func_ioctl) (int, unsigned long, void *) = NULL;
	va_list args;
	void *argp;
	
	if (!func_ioctl)
		func_ioctl = (int (*) (int, unsigned long, void *)) dlsym(RTLD_NEXT, "ioctl");
	
	va_start(args, request);
	argp = va_arg (args, void *);
	va_end(args);
	
	if (request == TIOCSSERIAL || request == TIOCGSERIAL) {
		struct serial_struct *nuts = (struct serial_struct *) argp;
		if (request == TIOCGSERIAL) {
			int ret = func_ioctl(fd, request, argp);
			nuts->baud_base = 768000000; // LCM(1600000, 1500000, 1228800)
			return ret;
		} else {
			speed_hack = nuts->baud_base / nuts->custom_divisor;
			printf("HOOK TIOCSSERIAL: baud_base=%d, custom_divisor=%d, speed=%d, fd=%d\n", nuts->baud_base, nuts->custom_divisor, nuts->baud_base / nuts->custom_divisor, fd);
			return 0;
		}
	}
	
	return func_ioctl(fd, request, argp);
}

unsigned cfgetospeed(const struct termios *termios_p) {
	static int (*func_cfgetospeed) (const struct termios *termios_p) = NULL;
	if (!func_cfgetospeed)
		func_cfgetospeed = (int (*) (const struct termios *termios_p)) dlsym(RTLD_NEXT, "cfgetospeed");
	unsigned speed = func_cfgetospeed(termios_p);
	if (speed == 0x1000) {
		printf("cfgetospeed = %x\n", speed);
		return B0;
	}
	return speed;
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p) {
	static int (*func_tcsetattr) (int fd, int optional_actions, const struct termios *termios_p) = NULL;
	if (enable_speed_hack) {
		struct termios2 tio;
		if (ioctl(fd, TCGETS2, &tio) != 0)
			perror("TCGETS2 hack");
		tio.c_cflag &= ~CBAUD;
		tio.c_cflag |= CBAUDEX;
		tio.c_ispeed = speed_hack;
		tio.c_ospeed = speed_hack;
		if (ioctl(fd, TCSETS2, &tio) != 0)
			perror("TCSETS2 hack");
		return 0;
	}
	if (!func_tcsetattr)
		func_tcsetattr = (int (*) (int fd, int optional_actions, const struct termios *termios_p)) dlsym(RTLD_NEXT, "tcsetattr");
	return func_tcsetattr(fd, optional_actions, termios_p);
}

int cfsetospeed(struct termios *termios_p, unsigned speed) {
	static int (*func_cfsetospeed) (struct termios *termios_p, unsigned speed) = NULL;
	if (!func_cfsetospeed)
		func_cfsetospeed = (int (*) (struct termios *termios_p, unsigned speed)) dlsym(RTLD_NEXT, "cfsetospeed");
	enable_speed_hack = speed == B38400;
	return func_cfsetospeed(termios_p, speed);
}
