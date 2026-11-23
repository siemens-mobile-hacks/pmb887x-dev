#include <pmb887x.h>
#include <printf/printf.h>
#include <stdarg.h>

#include "sio.h"

#define PRINTF_BUFFER_SIZE 64

struct printf_buffer {
	char data[PRINTF_BUFFER_SIZE];
	uint32_t size;
};

static void printf_buffer_putc(char character, void *argument) {
	struct printf_buffer *buffer = argument;
	buffer->data[buffer->size++] = character;
	if (buffer->size == sizeof(buffer->data)) {
		sio_write(buffer->data, buffer->size);
		buffer->size = 0;
	}
}

int vprintf(const char *format, va_list arguments) {
	struct printf_buffer buffer = { .size = 0 };
	int length = vfctprintf(printf_buffer_putc, &buffer, format, arguments);
	sio_write(buffer.data, buffer.size);
	return length;
}

int printf(const char *format, ...) {
	va_list arguments;
	va_start(arguments, format);
	int length = vprintf(format, arguments);
	va_end(arguments);
	return length;
}
