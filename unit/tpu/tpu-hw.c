#include <pmb887x.h>
#include <stopwatch.h>

#include "tpu/tpu-hw.h"
#include "test.h"

#define TPU_TIMER_RAM_BASE 512
#define TPU_DECODER_SHIFT 6
#define TPU_EVENT_TICK 100
#define TPU_EVENT_SPACING_TICKS 20
#define TPU_TRAILING_DELAY_TICKS 100
#define TPU_FRAME_TICKS 1000
#define TPU_TIMEOUT_MS 100

static bool wait_event_pointer(uint32_t value) {
	stopwatch_t start = stopwatch_get();

	while (TPU_CEAP != value && stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS)
		test_watchdog_serve();

	return TPU_CEAP == value;
}

void tpu_hw_configure_decoders(const uint16_t *decoders, size_t count) {
	TPU_PARAM = 0;
	TPU_TGER = 0;
	TPU_EAPT = 0;
	TPU_EAPB = 0;
	TPU_OFFSET = 0;
	TPU_PARAM = TPU_PARAM_TINI;
	TPU_PARAM = 0;

	TPU_GSMCLK1 = 1 << TPU_GSMCLK1_K_SHIFT;
	TPU_GSMCLK2 = 32 << TPU_GSMCLK2_L_SHIFT;
	TPU_GSMCLK3 = TPU_GSMCLK3_LOAD | TPU_GSMCLK3_INIT;
	TPU_OVERFLOW = TPU_FRAME_TICKS - 1;
	TPU_OFFSET = 0;
	TPU_TGER = BIT(0);
	TPU_EAPB = 0;
	TPU_EAPT = (count + 1) * 3;
	for (size_t i = 0; i < count; i++) {
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3) = 0;
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3 + 1) = TPU_EVENT_TICK + i * TPU_EVENT_SPACING_TICKS;
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3 + 2) = decoders[i] << TPU_DECODER_SHIFT;
	}
	TPU_RAM(TPU_TIMER_RAM_BASE + count * 3) = 0;
	TPU_RAM(TPU_TIMER_RAM_BASE + count * 3 + 1) = TPU_EVENT_TICK + count * TPU_EVENT_SPACING_TICKS + TPU_TRAILING_DELAY_TICKS;
	TPU_RAM(TPU_TIMER_RAM_BASE + count * 3 + 2) = 0;
}

bool tpu_hw_execute_decoders(size_t count) {
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	bool completed = wait_event_pointer(count * 3) && wait_event_pointer(0);
	TPU_PARAM = 0;
	return completed;
}

bool tpu_hw_run_decoder(uint16_t decoder) {
	tpu_hw_configure_decoders(&decoder, 1);
	return tpu_hw_execute_decoders(1);
}
