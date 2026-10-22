/* I2Cv2 bus timing of PASIC reads, measured with the STM. */

#include <pmb887x.h>

#include "dmac.h"
#include "driver.h"
#include "test.h"

#ifdef PMB8876

#include <pmic/PASIC.h>

#define TIMING_RUNS 8
#define DMA_CHANNEL 1
#define MAX_READ 32

struct timing_config {
	const char *name;
	uint32_t rmc;
	uint32_t dec;
	uint32_t inc;
};

static const struct timing_config CONFIGS[] = {
	{"firmware: RMC=4 DEC=0x3D INC=4", 4, 0x3D, 4},
	{"RMC=1 DEC=0x3D INC=4", 1, 0x3D, 4},
	{"RMC=4 DEC=0x0A INC=1", 4, 0x0A, 1},
	{"slow: RMC=8 DEC=0x7F INC=1", 8, 0x7F, 1},
};

static uint32_t ticks_to_ns(stopwatch_t ticks) {
	return (uint32_t) (ticks * 1000 / stopwatch_ticks_per_us());
}

/* Shortest of several runs, 0 if a read failed. */
static uint32_t polled_read_ns(uint32_t size) {
	uint8_t data[MAX_READ];
	uint32_t best = UINT32_MAX;

	for (uint32_t i = 0; i < TIMING_RUNS; i++) {
		cpu_enable_irq(false);
		stopwatch_t start = stopwatch_get();
		i2c_v2_result_t result = i2c_v2_smbus_read(PASIC_I2C_ADDR, PASIC_TURNOFF_REASON, data, size);
		uint32_t ns = ticks_to_ns(stopwatch_elapsed(start));
		cpu_enable_irq(true);

		if (result != I2C_V2_DONE)
			return 0;
		best = MIN(best, ns);
		test_watchdog_serve();
	}
	return best;
}

/* Shortest of several runs, 0 if a read failed. */
static uint32_t dma_read_ns(uint32_t size) {
	uint8_t data[MAX_READ];
	uint32_t best = UINT32_MAX;

	for (uint32_t i = 0; i < TIMING_RUNS; i++) {
		stopwatch_t start = stopwatch_get();
		i2c_v2_result_t result = i2c_v2_dma_read(DMA_CHANNEL, PASIC_I2C_ADDR, data, size);
		uint32_t ns = ticks_to_ns(stopwatch_elapsed(start));

		if (result != I2C_V2_DONE) {
			printf("# DMA read of %lu bytes failed: result=%u protocol=%08lX error=%08lX\n", size, result,
				i2c_v2_state.protocol_status, i2c_v2_state.error_status);
			return 0;
		}
		best = MIN(best, ns);
		test_watchdog_serve();
	}
	return best;
}

static void print_line(const char *what, uint32_t short_size, uint32_t short_ns, uint32_t long_size,
		uint32_t long_ns) {
	uint32_t byte_ns = (long_ns - short_ns) / (long_size - short_size);

	printf("#   %s: %lu bytes %lu ns, %lu bytes %lu ns -> %lu ns per byte, %lu ns besides the data bytes\n",
		what, short_size, short_ns, long_size, long_ns, byte_ns, short_ns - short_size * byte_ns);
}

static void test_config(const struct timing_config *config) {
	I2C_RUNCTRL = 0;
	I2C_CLC = config->rmc << MOD_CLC_RMC_SHIFT;
	I2C_FDIVCFG = (config->dec << I2C_FDIVCFG_DEC_SHIFT) | (config->inc << I2C_FDIVCFG_INC_SHIFT);
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	uint32_t scl_ns = (uint32_t) ((uint64_t) (2 * config->dec + 3 * config->inc) * config->rmc * 1000000000 /
		config->inc / cpu_get_ahb_per_freq());
	uint32_t polled1 = polled_read_ns(1);
	uint32_t polled8 = polled_read_ns(8);
	uint32_t dma1 = dma_read_ns(1);
	uint32_t dma8 = dma_read_ns(8);
	uint32_t dma32 = dma_read_ns(32);

	test_check("PASIC register reads complete", polled1 && polled8 && dma1 && dma8 && dma32);
	printf("# %s: SCL period %lu ns by the divider formula, 9 periods %lu ns\n", config->name, scl_ns, 9 * scl_ns);
	print_line("register read (START, address, register, repeated START, address, data, STOP), polled",
		1, polled1, 8, polled8);
	print_line("plain read (START, address, data, STOP), DMA", 1, dma1, 32, dma32);
	print_line("plain read, DMA", 8, dma8, 32, dma32);
	printf("# raw: rmc=%lu dec=%lu inc=%lu scl_ns=%lu polled1=%lu polled8=%lu dma1=%lu dma8=%lu dma32=%lu\n",
		config->rmc, config->dec, config->inc, scl_ns, polled1, polled8, dma1, dma8, dma32);
}

int main(void) {
	test_start("I2Cv2 bus timing test");
	printf("# CGU: OSC=%08lX CON0=%08lX CON1=%08lX CON3=%08lX; AHB_PER %lu Hz; STM %lu ticks/us\n",
		CGU_OSC, CGU_CON0, CGU_CON1, CGU_CON3, cpu_get_ahb_per_freq(), stopwatch_ticks_per_us());
	i2c_v2_init();
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	dmac_request_select(I2C_DMA_RX_REQUEST, I2C_DMA_RX_SELECT);

	for (uint32_t i = 0; i < ARRAY_SIZE(CONFIGS); i++) {
		test_category(CONFIGS[i].name);
		test_config(&CONFIGS[i]);
	}

	return test_finish();
}

__IRQ void irq_handler(void) {
	i2c_v2_handle_irq(VIC_IRQ_CURRENT);
	VIC_IRQ_ACK = 1;
}

#else

int main(void) {
	test_start("I2Cv2 bus timing test");
	test_skip("I2Cv2 bus timing", "unsupported");

	return test_finish();
}

#endif
