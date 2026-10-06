#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"
#include "test.h"

#define DSP_FC_PREPARE_FLAG BIT(7)
#define DSP_FC_RESULT_FLAGS (BIT(3) | BIT(4))
// Extended runtime Shared RAM ABI used by its frequency-correction handler.
#define DSP_FC_STATUS_OFFSET 89
#define DSP_FC_START_OFFSET 90
#define DSP_FC_QUALITY_OFFSET 91
#define DSP_FC_RMS_OFFSET 92
#define DSP_FC_FREQUENCY_OFFSET 93
#define DSP_FC_SENTINEL_BASE 0xA500
#define DSP_NOTIFICATION_SAMPLES 64
#define DSP_NOTIFICATION_TIMEOUT_MS 20
#define DSP_PREPARE_OBSERVATION_US 5000
#define DSP_FCON_HOLD_US 1000

#define TPU_TIMER_RAM_BASE 512
#define TPU_EVENT_WORDS 3
#define TPU_EVENT_GROUP_SHIFT 9
#define TPU_EVENT_TRIGGER_HIGH_SHIFT 16
#define TPU_EVENT_TRIGGER_HIGH_MASK 0x1FF
#define TPU_EVENT_TIME_TRIGGER_SHIFT 15
#define TPU_EVENT_TRIGGER_25_SHIFT 25
#define TPU_EVENT_TRIGGER_LOW_MASK 0xFFFF
#define TPU_FRAME_TICKS 10000

#define TPU_EVENT_SET 0x4000
#define TPU_EVENT_CLEAR 0x8000
#define TPU_DECODER_SHIFT 6
#define TPU_DECODER_FRAME_INT 15
#define TPU_DECODER_FCON_SET 4
#define TPU_DECODER_RECEIVE_CLEAR 5
#define TPU_TRIGGER_DECODER(value) ((value) << TPU_DECODER_SHIFT)
#define TPU_TRIGGER_T_OUT_0 BIT(12)

#define TPU_FC_PREPARE_GROUPS 0x8000C041
#define TPU_FC_START_GROUPS 0x8000C003

struct tpu_event {
	uint16_t operation;
	uint8_t group;
	uint16_t tick;
	uint32_t triggers;
};

// Minimal RX-only subset of the firmware frequency-correction TPU schedule.
static const struct tpu_event FC_EVENTS[] = {
	{ TPU_EVENT_SET, 0, 0x0001, TPU_TRIGGER_DECODER(TPU_DECODER_FRAME_INT) },
	{ TPU_EVENT_CLEAR, 6, 0x0008, TPU_TRIGGER_DECODER(TPU_DECODER_RECEIVE_CLEAR) },
	{ TPU_EVENT_SET, 1, 0x0256, TPU_TRIGGER_DECODER(TPU_DECODER_FCON_SET) },
	{ TPU_EVENT_CLEAR, 6, 0x1120, TPU_TRIGGER_DECODER(TPU_DECODER_RECEIVE_CLEAR) },
	{ TPU_EVENT_SET, 31, 0x7FF8, TPU_TRIGGER_T_OUT_0 },
};

static void tpu_schedule_write(void) {
	for (size_t i = 0; i < ARRAY_SIZE(FC_EVENTS); i++) {
		uint32_t triggers = FC_EVENTS[i].triggers;
		uint16_t control = FC_EVENTS[i].operation | (FC_EVENTS[i].group << TPU_EVENT_GROUP_SHIFT) |
			((triggers >> TPU_EVENT_TRIGGER_HIGH_SHIFT) & TPU_EVENT_TRIGGER_HIGH_MASK);
		uint16_t tick = FC_EVENTS[i].tick |
			(((triggers >> TPU_EVENT_TRIGGER_25_SHIFT) & 1) << TPU_EVENT_TIME_TRIGGER_SHIFT);
		uint32_t offset = TPU_TIMER_RAM_BASE + i * TPU_EVENT_WORDS;

		TPU_RAM(offset) = control;
		TPU_RAM(offset + 1) = tick;
		TPU_RAM(offset + 2) = triggers & TPU_EVENT_TRIGGER_LOW_MASK;
	}
}

static void tpu_window_start(uint32_t groups) {
	TPU_PARAM = 0;
	TPU_EAPT = ARRAY_SIZE(FC_EVENTS) * TPU_EVENT_WORDS;
	TPU_EAPB = 0;
	TPU_TGER = groups;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
}

static bool wait_for_flags(uint16_t mask, uint16_t expected, uint32_t *elapsed_ticks) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < DSP_NOTIFICATION_TIMEOUT_MS) {
		if ((DSP_COM_STATUS & mask) == expected) {
			*elapsed_ticks = (uint32_t) stopwatch_elapsed(start);
			return true;
		}
		test_watchdog_serve();
	}

	*elapsed_ticks = (uint32_t) stopwatch_elapsed(start);
	return false;
}

static bool runtime_start(void) {
	const struct dsp_user_rom *user_rom;

	return dsp_runtime_start(&user_rom) && user_rom->supports_fc_notifications;
}

static void test_fc_notifications(void) {
	static const uint16_t FC_PARAMETERS[] = { 1, 115, 33 };
	uint32_t start_max = 0;
	uint32_t finish_max = 0;
	uint32_t completed_samples = 0;
	bool success = true;

	if (!test_check("firmware FC_INIT command is accepted",
		dsp_runtime_command_execute(0, DSP_COMMAND_FC_INIT, FC_PARAMETERS, ARRAY_SIZE(FC_PARAMETERS), true))) {
		return;
	}

	DSP_COM_SET = DSP_FC_PREPARE_FLAG;
	for (size_t sample = 0; sample < DSP_NOTIFICATION_SAMPLES; sample++) {
		dsp_hw_shared_memory[DSP_FC_STATUS_OFFSET] = DSP_FC_SENTINEL_BASE;
		dsp_hw_shared_memory[DSP_FC_START_OFFSET] = DSP_FC_SENTINEL_BASE + 1;
		dsp_hw_shared_memory[DSP_FC_QUALITY_OFFSET] = DSP_FC_SENTINEL_BASE + 2;
		dsp_hw_shared_memory[DSP_FC_RMS_OFFSET] = DSP_FC_SENTINEL_BASE + 3;
		dsp_hw_shared_memory[DSP_FC_FREQUENCY_OFFSET] = DSP_FC_SENTINEL_BASE + 4;

		tpu_window_start(TPU_FC_PREPARE_GROUPS);
		stopwatch_usleep_wd(DSP_PREPARE_OBSERVATION_US);
		bool prepare_pending = (DSP_COM_STATUS & (DSP_FC_PREPARE_FLAG | DSP_FC_RESULT_FLAGS)) ==
			DSP_FC_PREPARE_FLAG;

		uint32_t elapsed;
		tpu_window_start(TPU_FC_START_GROUPS);
		bool started = wait_for_flags(DSP_FC_PREPARE_FLAG | DSP_FC_RESULT_FLAGS,
			DSP_FC_PREPARE_FLAG | DSP_FC_RESULT_FLAGS, &elapsed);
		if (elapsed > start_max)
			start_max = elapsed;
		stopwatch_usleep_wd(DSP_FCON_HOLD_US);

		tpu_window_start(TPU_FC_PREPARE_GROUPS);
		bool finished = wait_for_flags(DSP_FC_PREPARE_FLAG | DSP_FC_RESULT_FLAGS,
			DSP_FC_PREPARE_FLAG, &elapsed);
		if (elapsed > finish_max)
			finish_max = elapsed;
		success &= prepare_pending && started && finished;
		if (!success)
			break;
		completed_samples++;
	}

	TPU_PARAM = 0;
	test_check("FC communication and result flags complete every scheduler sample without an MCU interrupt",
		success);
	bool result_complete = dsp_hw_shared_memory[DSP_FC_STATUS_OFFSET] == 0 &&
		dsp_hw_shared_memory[DSP_FC_START_OFFSET] == DSP_FC_SENTINEL_BASE + 1 &&
		dsp_hw_shared_memory[DSP_FC_QUALITY_OFFSET] == DSP_FC_SENTINEL_BASE + 2 &&
		dsp_hw_shared_memory[DSP_FC_RMS_OFFSET] == DSP_FC_SENTINEL_BASE + 3 &&
		dsp_hw_shared_memory[DSP_FC_FREQUENCY_OFFSET] == DSP_FC_SENTINEL_BASE + 4;
	test_check("zero-input FC search publishes only its status before clearing result flags", result_complete);
	printf("# FC notifications: samples=%lu start_max=%lu finish_max=%lu STM ticks "
		"status=%04lX start=%04lX quality=%04lX rms=%04lX frequency=%04lX\n",
		completed_samples, start_max, finish_max,
		(uint32_t) dsp_hw_shared_memory[DSP_FC_STATUS_OFFSET],
		(uint32_t) dsp_hw_shared_memory[DSP_FC_START_OFFSET],
		(uint32_t) dsp_hw_shared_memory[DSP_FC_QUALITY_OFFSET],
		(uint32_t) dsp_hw_shared_memory[DSP_FC_RMS_OFFSET],
		(uint32_t) dsp_hw_shared_memory[DSP_FC_FREQUENCY_OFFSET]);
}

int main(void) {
	test_start("DSP runtime notification test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (test_check("complete runtime starts for notification tests", runtime_start())) {
		tpu_schedule_write();
		TPU_GSMCLK1 = 1 << TPU_GSMCLK1_K_SHIFT;
		TPU_GSMCLK2 = 4 << TPU_GSMCLK2_L_SHIFT;
		TPU_GSMCLK3 = TPU_GSMCLK3_INIT;
		TPU_OVERFLOW = TPU_FRAME_TICKS - 1;
		TPU_OFFSET = 0;
		test_fc_notifications();
	}

	TPU_PARAM = 0;
	return test_finish();
}
