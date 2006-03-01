#ifndef _KERNEL_IO_DEV_KEYBOARD_H_
#define _KERNEL_IO_DEV_KEYBOARD_H_

#include <sys/types.h>
#include <kernel/io/io.h>

#define KEYBORAD_KEY_LCTRL		0x1D
#define KEYBORAD_KEY_LALT		0x38
#define KEYBORAD_KEY_LSHIFT		0x2A
#define KEYBORAD_KEY_RSHIFT		0x36
#define KEYBORAD_KEY_CAPS		0x3A
#define KEYBORAD_KEY_F1			0x3B
#define KEYBORAD_KEY_F2			0x3C
#define KEYBORAD_KEY_F3			0x3D
#define KEYBORAD_KEY_F4			0x3E
#define KEYBORAD_KEY_NUM		0x45
#define KEYBORAD_KEY_SCROLL		0x46

#define KEYBOARD_DATAREG		0x60
#define KEYBOARD_CONTROLREG		0x64
#define KEYBOARD_ACK			0xFA

#define KEYBOARD_LED_SCROLL		1
#define KEYBOARD_LED_NUM		2
#define KEYBOARD_LED_CAPS		4

int keyboard_init( void );

#endif
