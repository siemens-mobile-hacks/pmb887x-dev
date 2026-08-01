#include <pmb887x.h>
#include <gen/dsp.h>

#include "baseband-functional-8876.inc"
#include "dsp-hw.h"
#include "test.h"

#define TPU_TIMER_RAM_BASE 512
#define TPU_DECODER_SHIFT 6
#define TPU_TIMEOUT_MS 100
#define READY_MARKER 0xA55A
#define COMPLETE_MARKER 0x5AA5
#define READY_OFFSET 0x000F
#define COMMAND_OFFSET 0x0010
#define INT_POINTER_OFFSET 0x0011
#define EXPECTED_IRQS_OFFSET 0x0012
#define CONTROL_OR_OFFSET 0x0013
#define CONFIGURED_OFFSET 0x0014
#define IRQ_COUNT_OFFSET 0x0015
#define CONTROL_AND_OFFSET 0x0016
#define BRFILTER_CTRL_OFFSET 0x0017
#define COEFFICIENT0_OFFSET 0x0018
#define FINAL_CTRL_OFFSET 0x0020
#define FINAL_STATUS_OFFSET 0x0021
#define FINAL_POINTER_OFFSET 0x0022
#define FINAL_FLAGS_OFFSET 0x0023
#define FINAL_BRFILTER_CTRL_OFFSET 0x0024
#define FLAGS_BASE 0x0100
#define STATUS_BASE 0x0120
#define POINTER_BASE 0x0140
#define RAM_SNAPSHOT_BASE 0x0160
#define RAM_SNAPSHOT_WORDS 16
#define RAM_SENTINEL 0x5AA5
#define BASEBAND_IRQ_MASK (TEAK_INT_FINTA0_BBHI | TEAK_INT_FINTA0_BBLO | TEAK_INT_FINTA0_BB_FULL)

struct tpu_event {
	uint16_t time;
	uint16_t decoder;
};

struct baseband_scenario {
	const char *name;
	const struct tpu_event *events;
	size_t event_count;
	uint16_t interrupt_pointer;
	uint16_t expected_irqs;
	uint16_t control_or;
	uint16_t control_and;
	uint16_t brfilter_ctrl;
	uint16_t coefficient0;
};

static const struct tpu_event JOB_EVENTS[] = {
	{ 100, 6 },
	{ 400, 1 },
	{ 700, 5 },
	{ 800, 2 },
	{ 1100, 5 },
	{ 1200, 3 },
	{ 1500, 5 },
	{ 1600, 4 },
	{ 1900, 5 },
	{ 2000, 7 },
};

static const struct tpu_event STOP_EVENTS[] = {
	{ 100, 6 },
	{ 400, 1 },
	{ 1000, 5 },
	{ 1100, 7 },
};

static const struct tpu_event WRAP_EVENTS[] = {
	{ 100, 6 },
	{ 400, 1 },
	{ 900, 5 },
	{ 1000, 7 },
};

static const struct tpu_event CLEANUP_EVENTS[] = {
	{ 100, 5 },
	{ 200, 7 },
};

static const struct tpu_event DECIMATION_EVENTS[] = {
	{ 100, 6 },
	{ 400, 1 },
	{ 700, 5 },
	{ 800, 7 },
};

static const struct baseband_scenario JOB_SCENARIO = {
	"job signals, buffer writes, and interrupt pointer",
	JOB_EVENTS,
	ARRAY_SIZE(JOB_EVENTS),
	4,
	12,
	0,
	UINT16_MAX,
	0,
	0,
};

static const struct baseband_scenario STOP_SCENARIO = {
	"BB_STOP hardware override",
	STOP_EVENTS,
	ARRAY_SIZE(STOP_EVENTS),
	4,
	2,
	TEAK_BB_CTRL_BB_STOP,
	UINT16_MAX,
	0,
	0,
};

static const struct baseband_scenario WRAP_SCENARIO = {
	"960-word sample-buffer wrap",
	WRAP_EVENTS,
	ARRAY_SIZE(WRAP_EVENTS),
	959,
	3,
	0,
	UINT16_MAX,
	0,
	0,
};

static const struct baseband_scenario DECIMATION_SCENARIO = {
	"two-fold broad-filter decimation",
	DECIMATION_EVENTS,
	ARRAY_SIZE(DECIMATION_EVENTS),
	4,
	3,
	0,
	(uint16_t) ~TEAK_BB_CTRL_BBADAP_EN,
	TEAK_BB_BRFILTER_CTRL_DECIMATION,
	0x7FFF,
};

static uint16_t normal_job_pointer;

static void configure_tpu(const struct tpu_event *events, size_t count) {
	TPU_PARAM = 0;
	TPU_GSMCLK1 = 1 << TPU_GSMCLK1_K_SHIFT;
	TPU_GSMCLK2 = 32 << TPU_GSMCLK2_L_SHIFT;
	TPU_GSMCLK3 = TPU_GSMCLK3_LOAD | TPU_GSMCLK3_INIT;
	TPU_OVERFLOW = 5999;
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

static void cleanup_tpu_signals(void) {
	configure_tpu(CLEANUP_EVENTS, ARRAY_SIZE(CLEANUP_EVENTS));
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	stopwatch_usleep_wd(1000);
	TPU_PARAM = 0;
}

static void print_record(const struct baseband_scenario *scenario, size_t pass) {
	printf("# BASEBAND,%s,pass=%u,irqs=%04X,ctrl=%04X,status=%04X,pointer=%04X,flags=%04X,br=%04X\n",
		scenario->name, (uint32_t) pass, (uint32_t) dsp_hw_shared_memory[IRQ_COUNT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[FINAL_CTRL_OFFSET],
		(uint32_t) dsp_hw_shared_memory[FINAL_STATUS_OFFSET],
		(uint32_t) dsp_hw_shared_memory[FINAL_POINTER_OFFSET],
		(uint32_t) dsp_hw_shared_memory[FINAL_FLAGS_OFFSET],
		(uint32_t) dsp_hw_shared_memory[FINAL_BRFILTER_CTRL_OFFSET]);
	for (size_t i = 0; i < scenario->expected_irqs; i++) {
		printf("# BASEBAND,event=%u,flags=%04X,status=%04X,pointer=%04X\n", (uint32_t) i,
			(uint32_t) dsp_hw_shared_memory[FLAGS_BASE + i],
			(uint32_t) dsp_hw_shared_memory[STATUS_BASE + i],
			(uint32_t) dsp_hw_shared_memory[POINTER_BASE + i]);
	}
}

static void validate_common(const struct baseband_scenario *scenario) {
	test_eq_u32("all expected baseband interrupts enter the real INT0 handler", scenario->expected_irqs,
		dsp_hw_shared_memory[IRQ_COUNT_OFFSET]);
	test_eq_u32("RXON and all job signals are inactive at completion", 0,
		dsp_hw_shared_memory[FINAL_STATUS_OFFSET] & (TEAK_BB_STATUS_RXON | TEAK_BB_STATUS_EQON |
		TEAK_BB_STATUS_MONON | TEAK_BB_STATUS_SCON | TEAK_BB_STATUS_FCON));
	test_eq_u32("the handler acknowledges every baseband source", 0,
		dsp_hw_shared_memory[FINAL_FLAGS_OFFSET] & BASEBAND_IRQ_MASK);
	test_eq_u32("RXON low clears the BB_STOP hardware override", 0,
		dsp_hw_shared_memory[FINAL_CTRL_OFFSET] & TEAK_BB_CTRL_BB_STOP);
}

static void validate_job_signals(void) {
	static const uint16_t JOB_BITS[] = {
		TEAK_BB_STATUS_EQON,
		TEAK_BB_STATUS_MONON,
		TEAK_BB_STATUS_SCON,
		TEAK_BB_STATUS_FCON,
	};

	for (size_t job = 0; job < ARRAY_SIZE(JOB_BITS); job++) {
		size_t high = job * 3;
		size_t full = high + 1;
		size_t low = high + 2;
		char name[96];

		tfp_sprintf(name, "job %u rising edge raises BBHI", (uint32_t) job);
		test_eq_u32(name, TEAK_INT_FINTA0_BBHI, dsp_hw_shared_memory[FLAGS_BASE + high] & BASEBAND_IRQ_MASK);
		tfp_sprintf(name, "job %u rising edge exposes its status and RXON", (uint32_t) job);
		test_eq_u32(name, TEAK_BB_STATUS_RXON | JOB_BITS[job],
			dsp_hw_shared_memory[STATUS_BASE + high] & (TEAK_BB_STATUS_RXON | TEAK_BB_STATUS_EQON |
			TEAK_BB_STATUS_MONON | TEAK_BB_STATUS_SCON | TEAK_BB_STATUS_FCON));
		tfp_sprintf(name, "job %u resets the write pointer below the interrupt position", (uint32_t) job);
		test_check(name, dsp_hw_shared_memory[POINTER_BASE + high] < 4);

		tfp_sprintf(name, "job %u reaches INT_POINTER and raises BB_FULL", (uint32_t) job);
		test_eq_u32(name, TEAK_INT_FINTA0_BB_FULL,
			dsp_hw_shared_memory[FLAGS_BASE + full] & BASEBAND_IRQ_MASK);
		tfp_sprintf(name, "job %u remains active when BB_FULL is delivered", (uint32_t) job);
		test_eq_u32(name, TEAK_BB_STATUS_RXON | JOB_BITS[job],
			dsp_hw_shared_memory[STATUS_BASE + full] & (TEAK_BB_STATUS_RXON | TEAK_BB_STATUS_EQON |
			TEAK_BB_STATUS_MONON | TEAK_BB_STATUS_SCON | TEAK_BB_STATUS_FCON));

		tfp_sprintf(name, "job %u falling edge raises BBLO", (uint32_t) job);
		test_eq_u32(name, TEAK_INT_FINTA0_BBLO, dsp_hw_shared_memory[FLAGS_BASE + low] & BASEBAND_IRQ_MASK);
		tfp_sprintf(name, "job %u falling edge clears only the job signal", (uint32_t) job);
		test_eq_u32(name, TEAK_BB_STATUS_RXON,
			dsp_hw_shared_memory[STATUS_BASE + low] & (TEAK_BB_STATUS_RXON | TEAK_BB_STATUS_EQON |
			TEAK_BB_STATUS_MONON | TEAK_BB_STATUS_SCON | TEAK_BB_STATUS_FCON));
		tfp_sprintf(name, "job %u keeps advancing after INT_POINTER until its falling edge", (uint32_t) job);
		test_check(name, dsp_hw_shared_memory[POINTER_BASE + low] >
			dsp_hw_shared_memory[POINTER_BASE + full]);
	}

	test_eq_u32("the final write pointer remains at the last completed job position",
		dsp_hw_shared_memory[POINTER_BASE + 11], dsp_hw_shared_memory[FINAL_POINTER_OFFSET]);
	if (normal_job_pointer == 0) {
		normal_job_pointer = dsp_hw_shared_memory[FINAL_POINTER_OFFSET];
	} else {
		test_eq_u32("normal-filter job length repeats after DSP reset", normal_job_pointer,
			dsp_hw_shared_memory[FINAL_POINTER_OFFSET]);
	}
	for (size_t i = 0; i < RAM_SNAPSHOT_WORDS; i++) {
		char name[80];

		tfp_sprintf(name, "sample RAM word %u is written by the active receive path", (uint32_t) i);
		test_check(name, dsp_hw_shared_memory[RAM_SNAPSHOT_BASE + i] != RAM_SENTINEL);
	}
}

static void validate_stop(void) {
	test_eq_u32("EQON rising edge still raises BBHI while BB_STOP is set", TEAK_INT_FINTA0_BBHI,
		dsp_hw_shared_memory[FLAGS_BASE] & BASEBAND_IRQ_MASK);
	test_eq_u32("EQON falling edge still raises BBLO while BB_STOP is set", TEAK_INT_FINTA0_BBLO,
		dsp_hw_shared_memory[FLAGS_BASE + 1] & BASEBAND_IRQ_MASK);
	test_eq_u32("BB_STOP prevents the write pointer from advancing", 0,
		dsp_hw_shared_memory[POINTER_BASE + 1]);
	for (size_t i = 0; i < RAM_SNAPSHOT_WORDS; i++) {
		char name[80];

		tfp_sprintf(name, "BB_STOP preserves sample RAM word %u", (uint32_t) i);
		test_eq_u32(name, RAM_SENTINEL, dsp_hw_shared_memory[RAM_SNAPSHOT_BASE + i]);
	}
}

static void validate_wrap(void) {
	test_eq_u32("long job rising edge raises BBHI", TEAK_INT_FINTA0_BBHI,
		dsp_hw_shared_memory[FLAGS_BASE] & BASEBAND_IRQ_MASK);
	test_eq_u32("long job reaches address 959 and raises BB_FULL", TEAK_INT_FINTA0_BB_FULL,
		dsp_hw_shared_memory[FLAGS_BASE + 1] & BASEBAND_IRQ_MASK);
	test_eq_u32("long job falling edge raises BBLO", TEAK_INT_FINTA0_BBLO,
		dsp_hw_shared_memory[FLAGS_BASE + 2] & BASEBAND_IRQ_MASK);
	test_check("write pointer wraps below 959 before the long job ends",
		dsp_hw_shared_memory[POINTER_BASE + 2] < 959);
	test_check("write pointer continues after wrapping to address zero",
		dsp_hw_shared_memory[POINTER_BASE + 2] != 0);
	test_eq_u32("wrapped pointer remains stable after RXON falls", dsp_hw_shared_memory[POINTER_BASE + 2],
		dsp_hw_shared_memory[FINAL_POINTER_OFFSET]);
}

static void validate_decimation(void) {
	test_eq_u32("decimation job rising edge raises BBHI", TEAK_INT_FINTA0_BBHI,
		dsp_hw_shared_memory[FLAGS_BASE] & BASEBAND_IRQ_MASK);
	test_eq_u32("decimation job reaches INT_POINTER and raises BB_FULL", TEAK_INT_FINTA0_BB_FULL,
		dsp_hw_shared_memory[FLAGS_BASE + 1] & BASEBAND_IRQ_MASK);
	test_eq_u32("decimation job falling edge raises BBLO", TEAK_INT_FINTA0_BBLO,
		dsp_hw_shared_memory[FLAGS_BASE + 2] & BASEBAND_IRQ_MASK);
	test_eq_u32("decimation job keeps RXON and EQON active at BB_FULL", TEAK_BB_STATUS_RXON | TEAK_BB_STATUS_EQON,
		dsp_hw_shared_memory[STATUS_BASE + 1] & (TEAK_BB_STATUS_RXON | TEAK_BB_STATUS_EQON));
	test_eq_u32("decimation mode halves the number of stored samples", normal_job_pointer,
		(uint32_t) dsp_hw_shared_memory[FINAL_POINTER_OFFSET] * 2);
	test_eq_u32("decimation mode remains selected after the job", TEAK_BB_BRFILTER_CTRL_DECIMATION,
		dsp_hw_shared_memory[FINAL_BRFILTER_CTRL_OFFSET]);
	test_eq_u32("decimation mode keeps the adaptive filter disabled", 0,
		dsp_hw_shared_memory[FINAL_CTRL_OFFSET] & TEAK_BB_CTRL_BBADAP_EN);
	for (size_t i = 0; i < RAM_SNAPSHOT_WORDS; i++) {
		char name[80];

		tfp_sprintf(name, "decimation path writes sample RAM word %u", (uint32_t) i);
		test_check(name, dsp_hw_shared_memory[RAM_SNAPSHOT_BASE + i] != RAM_SENTINEL);
	}
}

static bool run_scenario(const struct baseband_scenario *scenario, size_t pass) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load the baseband runner",
		dsp_hw_load_image(DSP_BASEBAND_FUNCTIONAL_8876, sizeof(DSP_BASEBAND_FUNCTIONAL_8876))))
	{
		return false;
	}
	if (!test_check("BRANCH starts the baseband runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("baseband runner becomes ready",
		dsp_hw_wait_shared(READY_OFFSET, READY_MARKER, TPU_TIMEOUT_MS)))
	{
		return false;
	}

	configure_tpu(scenario->events, scenario->event_count);
	dsp_hw_shared_memory[INT_POINTER_OFFSET] = scenario->interrupt_pointer;
	dsp_hw_shared_memory[EXPECTED_IRQS_OFFSET] = scenario->expected_irqs;
	dsp_hw_shared_memory[CONTROL_OR_OFFSET] = scenario->control_or;
	dsp_hw_shared_memory[CONTROL_AND_OFFSET] = scenario->control_and;
	dsp_hw_shared_memory[BRFILTER_CTRL_OFFSET] = scenario->brfilter_ctrl;
	dsp_hw_shared_memory[COEFFICIENT0_OFFSET] = scenario->coefficient0;
	dsp_hw_shared_memory[COMMAND_OFFSET] = 1;
	if (!test_check("baseband runner configures its interrupt and sample buffer",
		dsp_hw_wait_shared(CONFIGURED_OFFSET, READY_MARKER, TPU_TIMEOUT_MS)))
	{
		return false;
	}

	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	bool completed = dsp_hw_wait_shared(0, COMPLETE_MARKER, TPU_TIMEOUT_MS);
	TPU_PARAM = 0;
	cleanup_tpu_signals();
	print_record(scenario, pass);
	if (!test_check("baseband scenario completes", completed))
		return false;

	validate_common(scenario);
	if (scenario == &JOB_SCENARIO) {
		validate_job_signals();
	} else if (scenario == &STOP_SCENARIO) {
		validate_stop();
	} else if (scenario == &WRAP_SCENARIO) {
		validate_wrap();
	} else {
		validate_decimation();
	}
	return true;
}

int main(void) {
	test_start("DSP baseband receive functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	const struct baseband_scenario *scenarios[] = {
		&JOB_SCENARIO,
		&STOP_SCENARIO,
		&WRAP_SCENARIO,
		&DECIMATION_SCENARIO,
	};
	for (size_t i = 0; i < ARRAY_SIZE(scenarios); i++) {
		for (size_t pass = 1; pass <= 2; pass++) {
			char category[96];

			tfp_sprintf(category, "%s / independent reset pass %u", scenarios[i]->name, (uint32_t) pass);
			test_category(category);
			if (!run_scenario(scenarios[i], pass))
				goto finish;
		}
	}

finish:
	TPU_PARAM = 0;
	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
