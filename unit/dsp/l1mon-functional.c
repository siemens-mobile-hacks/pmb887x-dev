#include <pmb887x.h>
#include <gen/dsp.h>
#include <stopwatch.h>
#include <wdt.h>

#include "dsp-hw.h"
#include "l1mon-functional-8876.inc"
#include "test.h"

#define TPU_TIMER_RAM_BASE 512
#define TPU_DECODER_SHIFT 6
#define TPU_DECODER_MONON_SET 2
#define TPU_DECODER_RECEIVE_CLEAR 5
#define TPU_DECODER_RXON_SET 6
#define TPU_DECODER_RXON_CLEAR 7
#define TPU_ISOLATED_FRAME_TICKS 6000
#define TPU_BURST_FRAME_TICKS 10000
#define TPU_MONON_HIGH_TICK 400
#define TPU_RXON_HIGH_TICK 100
#define TPU_RXON_TAIL_TICKS 100
#define TPU_BURST_WINDOW_COUNT 8
#define TPU_BURST_RESULT_COUNT 10
#define TPU_BURST_CALLBACK_COUNT (TPU_BURST_RESULT_COUNT * 2)
#define READY_MARKER 0xA55A
#define READY_OFFSET 0x000F
#define RUNTIME_PIPE_OFFSET 0x0005
#define BBHI_CALLBACK_ARGUMENT_OFFSET 0x0020
#define BBHI_CALLBACK_COUNT_OFFSET 0x0021
#define BBLO_CALLBACK_ARGUMENT_OFFSET 0x0022
#define BBLO_CALLBACK_COUNT_OFFSET 0x0023
#define MONITOR_CALLBACK_COUNT_OFFSET 0x0024
#define SCHEDULER_CALLBACK_ARGUMENT_OFFSET 0x0025
#define SCHEDULER_CALLBACK_COUNT_OFFSET 0x0026
#define PENDING_INTERRUPTS_OFFSET 0x0028
#define BASEBAND_CONTROL_OFFSET 0x0029
#define BBHI_WRITE_POINTER_OFFSET 0x002A
#define BBLO_WRITE_POINTER_OFFSET 0x002B
#define COMMAND_CALLBACK_ARGUMENT_OFFSET 0x002C
#define COMMAND_CALLBACK_COUNT_OFFSET 0x002D
#define EVENT_LOG_COUNT_OFFSET 0x0030
#define OBSERVED_MON_INDEX_OFFSET 0x0031
#define EVENT_LOG_OFFSET 0x0400
#define EVENT_LOG_WORDS 3
#define TIMER2_DIVIDER 96
#define DSP_CLOCK_HZ 104000000
#define DSP_COMMAND_IDLE 14
#define DSP_COMMAND_RF_ADAPT 67
#define MON_INDEX_OFFSET 191
#define MON_VALUES_OFFSET 192
#define MON_VALUE_COUNT 8
#define BASEBAND_RING_WORDS 960
#define MON_SENTINEL_BASE 0x5100
#define MON_RAW_MIN -128
#define MON_RAW_MAX 1400
#define TEST_TIMEOUT_MS 100

struct tpu_event {
	uint16_t time;
	uint16_t decoder;
};

struct monitor_event {
	uint16_t source;
	uint16_t phase;
};

enum {
	MONITOR_EVENT_BBHI = 1,
	MONITOR_EVENT_BBLO,
	MONITOR_EVENT_ALGORITHM,
	MONITOR_EVENT_SCHEDULER,
	MONITOR_EVENT_PUBLICATION,
};

static const struct monitor_event MONITOR_EVENT_SEQUENCE[] = {
	{ MONITOR_EVENT_BBHI, 0 },
	{ MONITOR_EVENT_BBHI, 3 },
	{ MONITOR_EVENT_BBLO, 0 },
	{ MONITOR_EVENT_BBLO, 3 },
	{ MONITOR_EVENT_SCHEDULER, 1 },
	{ MONITOR_EVENT_SCHEDULER, 5 },
	{ MONITOR_EVENT_ALGORITHM, 0 },
	{ MONITOR_EVENT_ALGORITHM, 1 },
	{ MONITOR_EVENT_PUBLICATION, 0xFFFF },
};

// Hardware-valid TPU widths; TPU ticks are not interchangeable with the documented GSM symbol count.
static const uint16_t MONITOR_WIDTHS[] = { 30, 60, 190 };

static const uint16_t BURST_RXON_HIGH_TICKS[] = { 323, 1407, 2542, 3915, 5024, 6133, 7242, 8351 };
static const uint16_t BURST_MONON_HIGH_TICKS[] = { 605, 1715, 2824, 4065, 5174, 6283, 7392, 8501 };

static const struct tpu_event CLEANUP_EVENTS[] = {
	{ 100, TPU_DECODER_RECEIVE_CLEAR },
	{ 200, TPU_DECODER_RXON_CLEAR },
};

static uint16_t isolated_results[ARRAY_SIZE(MONITOR_WIDTHS)];

static const char *monitor_event_name(uint16_t source) {
	switch (source) {
		case MONITOR_EVENT_BBHI:
			return "BBHI";

		case MONITOR_EVENT_BBLO:
			return "BBLO";

		case MONITOR_EVENT_ALGORITHM:
			return "MON";

		case MONITOR_EVENT_SCHEDULER:
			return "SCHED";

		case MONITOR_EVENT_PUBLICATION:
			return "PUBLISH";

		default:
			return "UNKNOWN";
	}
}

static void check_monitor_events(size_t first, size_t window_count, uint16_t first_index) {
	size_t count = dsp_hw_shared_memory[EVENT_LOG_COUNT_OFFSET];
	size_t expected_count = window_count * ARRAY_SIZE(MONITOR_EVENT_SEQUENCE);

	test_eq_u32("Mask ROM records every expected monitoring checkpoint", expected_count, count - first);
	for (size_t i = first; i < count; i++) {
		size_t offset = EVENT_LOG_OFFSET + i * EVENT_LOG_WORDS;
		uint16_t source = dsp_hw_shared_memory[offset];
		uint16_t phase = dsp_hw_shared_memory[offset + 1];
		uint16_t timestamp = dsp_hw_shared_memory[offset + 2];
		uint16_t delta = i == first ? 0 : timestamp - dsp_hw_shared_memory[offset - 1];
		uint32_t delta_cycles = (uint32_t) delta * TIMER2_DIVIDER;
		uint32_t delta_ns = (uint64_t) delta_cycles * 1000000000 / DSP_CLOCK_HZ;
		size_t sequence_index = (i - first) % ARRAY_SIZE(MONITOR_EVENT_SEQUENCE);
		const struct monitor_event *expected = &MONITOR_EVENT_SEQUENCE[sequence_index];
		uint16_t expected_phase = expected->phase;
		char name[88];

		if (expected->source == MONITOR_EVENT_PUBLICATION)
			expected_phase = (first_index + (i - first) / ARRAY_SIZE(MONITOR_EVENT_SEQUENCE) + 1) % MON_VALUE_COUNT;

		printf("# L1MON_EVENT,window=%u,step=%u,source=%s,phase=%u,timer2=%u,delta=%u,delta_ns=%u\n",
			(uint32_t) ((i - first) / ARRAY_SIZE(MONITOR_EVENT_SEQUENCE)), (uint32_t) sequence_index,
			monitor_event_name(source), (uint32_t) phase, (uint32_t) timestamp, (uint32_t) delta, delta_ns);
		tfp_sprintf(name, "monitoring event %u has the expected source", (uint32_t) (i - first));
		test_eq_u32(name, expected->source, source);
		tfp_sprintf(name, "monitoring event %u has the expected phase", (uint32_t) (i - first));
		test_eq_u32(name, expected_phase, phase);
	}
}

static void configure_tpu(const struct tpu_event *events, size_t count, uint16_t frame_ticks) {
	TPU_PARAM = 0;
	TPU_GSMCLK1 = 1 << TPU_GSMCLK1_K_SHIFT;
	TPU_GSMCLK2 = 32 << TPU_GSMCLK2_L_SHIFT;
	TPU_GSMCLK3 = TPU_GSMCLK3_LOAD | TPU_GSMCLK3_INIT;
	TPU_OVERFLOW = frame_ticks - 1;
	TPU_OFFSET = 0;
	TPU_TGER = BIT(0);
	TPU_EAPB = 0;
	TPU_EAPT = count * 3;
	for (size_t i = 0; i < count; i++) {
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3) = 0;
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3 + 1) = events[i].time;
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3 + 2) = events[i].decoder << TPU_DECODER_SHIFT;
	}
}

static void cleanup_tpu(void) {
	configure_tpu(CLEANUP_EVENTS, ARRAY_SIZE(CLEANUP_EVENTS), TPU_ISOLATED_FRAME_TICKS);
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	stopwatch_usleep_wd(3000);
	TPU_PARAM = 0;
}

static bool submit_runtime_command(uint16_t command, const uint16_t *parameters, size_t count) {
	dsp_hw_shared_memory[RUNTIME_PIPE_OFFSET] = command;
	for (size_t i = 0; i < count; i++)
		dsp_hw_shared_memory[RUNTIME_PIPE_OFFSET + 1 + i] = parameters[i];
	DSP_COM_SET = BIT(0);
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;

	stopwatch_t start = stopwatch_get();
	while ((DSP_COM_STATUS & BIT(0)) != 0 && stopwatch_elapsed_ms(start) < TEST_TIMEOUT_MS)
		test_watchdog_serve();
	bool completed = (DSP_COM_STATUS & BIT(0)) == 0;
	if (!completed) {
		printf("# MASK_COMMAND,id=%u,response=%04X,status=%04X\n", (uint32_t) command,
			(uint32_t) dsp_hw_shared_memory[RUNTIME_PIPE_OFFSET], (uint32_t) DSP_COM_STATUS);
	}

	return completed;
}

static bool prepare_runner(void) {
	static const uint16_t RF_ADAPT_PARAMETERS[] = { 0, 0x0599, 1, TEAK_BB_CTRL_BBADAP_EN, 0 };

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;

	DSP_COM_CLEAR = 0xFFFF;
	if (!test_check("boot commands load the minimal Mask ROM harness",
		dsp_hw_load_image(DSP_L1MON_FUNCTIONAL_8876, sizeof(DSP_L1MON_FUNCTIONAL_8876)))) {
		return false;
	}
	if (!test_check("BRANCH starts the minimal Mask ROM harness", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("minimal Mask ROM harness becomes ready",
		dsp_hw_wait_shared(READY_OFFSET, READY_MARKER, TEST_TIMEOUT_MS))) {
		return false;
	}

	dsp_hw_shared_memory[COMMAND_CALLBACK_ARGUMENT_OFFSET] = 0;
	dsp_hw_shared_memory[COMMAND_CALLBACK_COUNT_OFFSET] = 0;
	if (!test_check("Mask ROM accepts the RF_ADAPT Baseband initialization",
		submit_runtime_command(DSP_COMMAND_RF_ADAPT, RF_ADAPT_PARAMETERS, ARRAY_SIZE(RF_ADAPT_PARAMETERS)))) {
		return false;
	}
	if (!test_eq_u32("RF_ADAPT executes its fixed Mask ROM handler once", 1,
		dsp_hw_shared_memory[COMMAND_CALLBACK_COUNT_OFFSET])) {
		return false;
	}
	if (!test_eq_u32("RF_ADAPT reaches fixed handler phase nine", 9,
		dsp_hw_shared_memory[COMMAND_CALLBACK_ARGUMENT_OFFSET])) {
		return false;
	}

	return test_check("Mask ROM accepts the IDLE runtime command",
		submit_runtime_command(DSP_COMMAND_IDLE, NULL, 0));
}

static void initialize_results(void) {
	dsp_hw_shared_memory[MON_INDEX_OFFSET] = 0;
	for (size_t i = 0; i < MON_VALUE_COUNT; i++)
		dsp_hw_shared_memory[MON_VALUES_OFFSET + i] = MON_SENTINEL_BASE + i;
	for (size_t i = 0; i < ARRAY_SIZE(isolated_results); i++)
		isolated_results[i] = 0;
	test_check("DSP observer sees the initialized monitoring index",
		dsp_hw_wait_shared(OBSERVED_MON_INDEX_OFFSET, 0, TEST_TIMEOUT_MS));
}

static void reset_path_observers(void) {
	for (size_t i = BBHI_CALLBACK_ARGUMENT_OFFSET; i <= SCHEDULER_CALLBACK_COUNT_OFFSET; i++)
		dsp_hw_shared_memory[i] = 0;
	dsp_hw_shared_memory[BBHI_WRITE_POINTER_OFFSET] = 0;
	dsp_hw_shared_memory[BBLO_WRITE_POINTER_OFFSET] = 0;
}

static void run_window(size_t index) {
	uint16_t width = MONITOR_WIDTHS[index];
	struct tpu_event events[] = {
		{ TPU_RXON_HIGH_TICK, TPU_DECODER_RXON_SET },
		{ TPU_MONON_HIGH_TICK, TPU_DECODER_MONON_SET },
		{ TPU_MONON_HIGH_TICK + width, TPU_DECODER_RECEIVE_CLEAR },
		{ TPU_MONON_HIGH_TICK + width + TPU_RXON_TAIL_TICKS, TPU_DECODER_RXON_CLEAR },
	};
	char category[64];

	tfp_sprintf(category, "Mask ROM monitoring / %u TPU ticks", (uint32_t) width);
	test_category(category);
	reset_path_observers();
	size_t first_event = dsp_hw_shared_memory[EVENT_LOG_COUNT_OFFSET];
	configure_tpu(events, ARRAY_SIZE(events), TPU_ISOLATED_FRAME_TICKS);
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	bool completed = dsp_hw_wait_shared(MON_INDEX_OFFSET, index + 1, TEST_TIMEOUT_MS);
	TPU_PARAM = 0;
	cleanup_tpu();

	printf("# L1MON_PATH,bbhi=%u/%u/%u,bblo=%u/%u/%u,scheduler=%u/%u,monitor=%u,index=%u,"
		"finta0=%04X,bb_ctrl=%04X\n",
		(uint32_t) dsp_hw_shared_memory[BBHI_CALLBACK_COUNT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[BBHI_CALLBACK_ARGUMENT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[BBHI_WRITE_POINTER_OFFSET],
		(uint32_t) dsp_hw_shared_memory[BBLO_CALLBACK_COUNT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[BBLO_CALLBACK_ARGUMENT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[BBLO_WRITE_POINTER_OFFSET],
		(uint32_t) dsp_hw_shared_memory[SCHEDULER_CALLBACK_COUNT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[SCHEDULER_CALLBACK_ARGUMENT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[MONITOR_CALLBACK_COUNT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[MON_INDEX_OFFSET],
		(uint32_t) dsp_hw_shared_memory[PENDING_INTERRUPTS_OFFSET],
		(uint32_t) dsp_hw_shared_memory[BASEBAND_CONTROL_OFFSET]);
	if (!test_check("MONON falling edge publishes a monitoring result through Mask ROM", completed))
		return;
	check_monitor_events(first_event, 1, index);

	test_eq_u32("one MONON window traverses both BBHI handler phases", 2,
		dsp_hw_shared_memory[BBHI_CALLBACK_COUNT_OFFSET]);
	test_eq_u32("BBHI finishes in fixed Mask ROM phase three", 3,
		dsp_hw_shared_memory[BBHI_CALLBACK_ARGUMENT_OFFSET]);
	test_eq_u32("BBHI observes four Baseband words already stored", 4,
		dsp_hw_shared_memory[BBHI_WRITE_POINTER_OFFSET]);
	test_eq_u32("one MONON window traverses both BBLO handler phases", 2,
		dsp_hw_shared_memory[BBLO_CALLBACK_COUNT_OFFSET]);
	test_eq_u32("BBLO finishes in fixed Mask ROM phase three", 3,
		dsp_hw_shared_memory[BBLO_CALLBACK_ARGUMENT_OFFSET]);
	test_eq_u32("BBLO observes exactly two Baseband words per active TPU tick", width * 2 % BASEBAND_RING_WORDS,
		dsp_hw_shared_memory[BBLO_WRITE_POINTER_OFFSET]);
	test_eq_u32("one MONON window traverses both scheduler insertion phases", 2,
		dsp_hw_shared_memory[SCHEDULER_CALLBACK_COUNT_OFFSET]);
	test_eq_u32("scheduler insertion finishes in fixed Mask ROM phase five", 5,
		dsp_hw_shared_memory[SCHEDULER_CALLBACK_ARGUMENT_OFFSET]);
	test_eq_u32("one MONON window traverses both monitoring algorithm phases", 2,
		dsp_hw_shared_memory[MONITOR_CALLBACK_COUNT_OFFSET]);
	test_eq_u32("Mask ROM acknowledges every Baseband interrupt source", 0,
		dsp_hw_shared_memory[PENDING_INTERRUPTS_OFFSET]);
	test_eq_u32("RF_ADAPT leaves adaptive Baseband filtering enabled", TEAK_BB_CTRL_BBADAP_EN,
		dsp_hw_shared_memory[BASEBAND_CONTROL_OFFSET]);

	uint16_t result = dsp_hw_shared_memory[MON_VALUES_OFFSET + index];
	int16_t signed_result = (int16_t) result;

	printf("# L1MON_RESULT,index=%u,width_ticks=%u,raw=%04X,signed=%d\n",
		(uint32_t) index, (uint32_t) width, (uint32_t) result, (int32_t) signed_result);
	test_check("Mask ROM replaces the selected result sentinel", result != MON_SENTINEL_BASE + index);
	test_check("monitoring result is in the documented RMS range",
		signed_result >= MON_RAW_MIN && signed_result <= MON_RAW_MAX);
	test_eq_u32("Mask ROM advances the result ring exactly once", index + 1,
		dsp_hw_shared_memory[MON_INDEX_OFFSET]);
	isolated_results[index] = result;
	for (size_t i = 0; i < MON_VALUE_COUNT; i++) {
		char name[80];

		tfp_sprintf(name, "monitoring window %u preserves result slot %u", (uint32_t) index, (uint32_t) i);
		uint16_t expected = i <= index ? isolated_results[i] : MON_SENTINEL_BASE + i;
		test_eq_u32(name, expected, dsp_hw_shared_memory[MON_VALUES_OFFSET + i]);
	}
}

static void run_burst(void) {
	struct tpu_event events[TPU_BURST_WINDOW_COUNT * 4];

	test_category("SL98 monitoring burst / eight plus two windows");
	reset_path_observers();
	dsp_hw_shared_memory[MON_INDEX_OFFSET] = 0;
	for (size_t i = 0; i < MON_VALUE_COUNT; i++) {
		char name[72];

		dsp_hw_shared_memory[MON_VALUES_OFFSET + i] = 0xFFFF;
		tfp_sprintf(name, "ARM writes 0xFFFF to monitoring result slot %u", (uint32_t) i);
		test_eq_u32(name, 0xFFFF, dsp_hw_shared_memory[MON_VALUES_OFFSET + i]);
	}
	test_eq_u32("ARM resets the monitoring result ring index", 0, dsp_hw_shared_memory[MON_INDEX_OFFSET]);
	test_check("DSP observer sees the reset monitoring index",
		dsp_hw_wait_shared(OBSERVED_MON_INDEX_OFFSET, 0, TEST_TIMEOUT_MS));
	size_t first_event = dsp_hw_shared_memory[EVENT_LOG_COUNT_OFFSET];

	for (size_t i = 0; i < TPU_BURST_WINDOW_COUNT; i++) {
		events[i * 4] = (struct tpu_event) { BURST_RXON_HIGH_TICKS[i], TPU_DECODER_RXON_SET };
		events[i * 4 + 1] = (struct tpu_event) { BURST_MONON_HIGH_TICKS[i], TPU_DECODER_MONON_SET };
		events[i * 4 + 2] = (struct tpu_event) { BURST_MONON_HIGH_TICKS[i] + 500,
			TPU_DECODER_RECEIVE_CLEAR };
		events[i * 4 + 3] = (struct tpu_event) { BURST_MONON_HIGH_TICKS[i] + 501, TPU_DECODER_RXON_CLEAR };
	}

	configure_tpu(events, ARRAY_SIZE(events), TPU_BURST_FRAME_TICKS);
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	stopwatch_t start = stopwatch_get();
	while (dsp_hw_shared_memory[MONITOR_CALLBACK_COUNT_OFFSET] < TPU_BURST_CALLBACK_COUNT &&
		stopwatch_elapsed_ms(start) < TEST_TIMEOUT_MS) {
		test_watchdog_serve();
	}
	bool completed = dsp_hw_shared_memory[MONITOR_CALLBACK_COUNT_OFFSET] == TPU_BURST_CALLBACK_COUNT;
	TPU_PARAM = 0;
	stopwatch_usleep_wd(1000);
	cleanup_tpu();

	printf("# L1MON_BURST,bbhi=%u/%u,bblo=%u/%u,scheduler=%u,monitor=%u,index=%u,finta0=%04X\n",
		(uint32_t) dsp_hw_shared_memory[BBHI_CALLBACK_COUNT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[BBHI_WRITE_POINTER_OFFSET],
		(uint32_t) dsp_hw_shared_memory[BBLO_CALLBACK_COUNT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[BBLO_WRITE_POINTER_OFFSET],
		(uint32_t) dsp_hw_shared_memory[SCHEDULER_CALLBACK_COUNT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[MONITOR_CALLBACK_COUNT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[MON_INDEX_OFFSET],
		(uint32_t) dsp_hw_shared_memory[PENDING_INTERRUPTS_OFFSET]);
	test_check("ten SL98 monitoring windows finish before the timeout", completed);
	check_monitor_events(first_event, TPU_BURST_RESULT_COUNT, 0);
	test_eq_u32("ten burst windows traverse every BBHI handler phase", TPU_BURST_CALLBACK_COUNT,
		dsp_hw_shared_memory[BBHI_CALLBACK_COUNT_OFFSET]);
	test_eq_u32("burst BBHI processing finishes in fixed Mask ROM phase three", 3,
		dsp_hw_shared_memory[BBHI_CALLBACK_ARGUMENT_OFFSET]);
	test_eq_u32("burst BBHI observes four Baseband words already stored", 4,
		dsp_hw_shared_memory[BBHI_WRITE_POINTER_OFFSET]);
	test_eq_u32("ten burst windows traverse every BBLO handler phase", TPU_BURST_CALLBACK_COUNT,
		dsp_hw_shared_memory[BBLO_CALLBACK_COUNT_OFFSET]);
	test_eq_u32("burst BBLO processing finishes in fixed Mask ROM phase three", 3,
		dsp_hw_shared_memory[BBLO_CALLBACK_ARGUMENT_OFFSET]);
	test_eq_u32("burst BBLO observes 1000 words wrapped in the Baseband ring", 40,
		dsp_hw_shared_memory[BBLO_WRITE_POINTER_OFFSET]);
	test_eq_u32("ten burst windows traverse every scheduler insertion phase", TPU_BURST_CALLBACK_COUNT,
		dsp_hw_shared_memory[SCHEDULER_CALLBACK_COUNT_OFFSET]);
	test_eq_u32("burst scheduler insertion finishes in fixed Mask ROM phase five", 5,
		dsp_hw_shared_memory[SCHEDULER_CALLBACK_ARGUMENT_OFFSET]);
	test_eq_u32("ten burst windows traverse every monitoring phase", TPU_BURST_CALLBACK_COUNT,
		dsp_hw_shared_memory[MONITOR_CALLBACK_COUNT_OFFSET]);
	test_eq_u32("ten burst results leave the monitoring ring index at two", 2,
		dsp_hw_shared_memory[MON_INDEX_OFFSET]);
	test_eq_u32("monitoring burst leaves only BB_FULL latched", TEAK_INT_FINTA0_BB_FULL,
		dsp_hw_shared_memory[PENDING_INTERRUPTS_OFFSET]);
	test_eq_u32("monitoring burst keeps adaptive Baseband filtering enabled", TEAK_BB_CTRL_BBADAP_EN,
		dsp_hw_shared_memory[BASEBAND_CONTROL_OFFSET]);
	for (size_t i = 0; i < MON_VALUE_COUNT; i++) {
		char name[72];
		uint16_t result = dsp_hw_shared_memory[MON_VALUES_OFFSET + i];
		int16_t signed_result = (int16_t) result;

		tfp_sprintf(name, "monitoring burst fills result slot %u", (uint32_t) i);
		test_check(name, result != 0xFFFF);
		tfp_sprintf(name, "monitoring burst result slot %u is in the documented RMS range", (uint32_t) i);
		test_check(name, signed_result >= MON_RAW_MIN && signed_result <= MON_RAW_MAX);
	}
}

static void run_pass(size_t pass) {
	char category[56];

	tfp_sprintf(category, "Independent Mask ROM boot / pass %u", (uint32_t) pass);
	test_category(category);
	if (!prepare_runner())
		return;

	initialize_results();

	for (size_t i = 0; i < ARRAY_SIZE(MONITOR_WIDTHS); i++)
		run_window(i);

	for (size_t i = ARRAY_SIZE(MONITOR_WIDTHS); i < MON_VALUE_COUNT; i++) {
		char name[72];

		tfp_sprintf(name, "unselected monitoring result slot %u remains intact", (uint32_t) i);
		test_eq_u32(name, MON_SENTINEL_BASE + i, dsp_hw_shared_memory[MON_VALUES_OFFSET + i]);
	}
}

int main(void) {
	test_start("DSP autonomous Mask ROM monitoring");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	wdt_set_max_execution_time(UINT32_MAX);

	for (size_t pass = 1; pass <= 2; pass++)
		run_pass(pass);

	if (prepare_runner())
		run_burst();

	TPU_PARAM = 0;
	DSP_COM_CLEAR = 0xFFFF;
	(void) dsp_hw_reset();
	return test_finish();
}
