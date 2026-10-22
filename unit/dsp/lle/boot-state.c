#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/dsp-hw.h"
#include "test.h"

#define BOOT_FLAG BIT(0)
#define BOOT_DATA_OFFSET 2
#define BOOT_RESULT_INDEX 3
#define DSP_BOOT_PREAD 3
#define BOOT_SAMPLE_LIMIT 1000000
#define BOOT_WATCHDOG_INTERVAL_MASK 0x3FFF
#define BOOT_COMMAND_TIMEOUT_US 10000
#define BOOT_PENDING_OBSERVATION_US 100
#ifdef PMB8876
// EL71 waits 0x1E STM ticks at 1 MHz for the DSP boot flag to clear.
#define EL71_FIRMWARE_STM_HZ 1000000
#define EL71_DSP_WAIT_TICKS 0x1E
#endif
#define FW_VERSION_OFFSET 0
#define HW_VERSION_OFFSET 1
#ifdef PMB8875
#define DSP_SUBSYSTEM_VERSION 0xE001
#define PROGRAM_ROM_FIRST 0x1000
#define PROGRAM_ROM_BRANCH_TARGET 0x1010
#else
#define DSP_SUBSYSTEM_VERSION 0xE101
#define PROGRAM_ROM_FIRST 0x2000
#define PROGRAM_ROM_BRANCH_TARGET 0x2010
#endif
#define RESULT_SENTINEL 0x5AA5
#define FW_VERSION_SENTINEL 0xDEAD
#define HW_VERSION_SENTINEL 0xBEEF

struct boot_observation {
	uint32_t busy_iteration;
	uint32_t ready_iteration;
	uint32_t elapsed_us;
	uint32_t status_before_reset;
	uint32_t status_after_ready;
	bool busy_seen;
	bool ready_seen;
};

static bool mask_rom_is_supported(uint16_t mask_id) {
#ifdef PMB8875
	return mask_id == 0x0602 || mask_id == 0x0604;
#else
	return mask_id == 0x0801;
#endif
}

#ifdef PMB8876
static void test_el71_boot_wait_budget(void) {
	DSP_COM_CLEAR = 0xFFFF;
	SCU_DSP_INT = 0;
	SCU_RST_REQ = SCU_RST_REQ_DSP;
	(void) SCU_RST_REQ;
	SCU_RST_REQ = 0;

	bool busy_seen = false;
	for (uint32_t i = 0; i < BOOT_SAMPLE_LIMIT; i++) {
		if ((DSP_COM_STATUS & BOOT_FLAG) != 0) {
			busy_seen = true;
			break;
		}
		if ((i & BOOT_WATCHDOG_INTERVAL_MASK) == BOOT_WATCHDOG_INTERVAL_MASK)
			test_watchdog_serve();
	}

	uint32_t test_stm_hz = stopwatch_ticks_per_s();
	stopwatch_t timeout_ticks = (stopwatch_t) EL71_DSP_WAIT_TICKS * test_stm_hz / EL71_FIRMWARE_STM_HZ;
	stopwatch_t start = stopwatch_get();
	bool ready;
	for (;;) {
		if ((DSP_COM_STATUS & BOOT_FLAG) == 0) {
			ready = true;
			break;
		}
		if (stopwatch_elapsed(start) > timeout_ticks) {
			ready = (DSP_COM_STATUS & BOOT_FLAG) == 0;
			break;
		}
		test_watchdog_serve();
	}
	stopwatch_t elapsed_ticks = stopwatch_elapsed(start);

	printf("# firmware DSP wait: firmware=%lu ticks at %lu Hz, test=%lu ticks at %lu Hz, elapsed=%lu\n",
		(uint32_t) EL71_DSP_WAIT_TICKS, (uint32_t) EL71_FIRMWARE_STM_HZ,
		(uint32_t) timeout_ticks, test_stm_hz, (uint32_t) elapsed_ticks);
	test_check("DSP reset publishes the busy communication flag", busy_seen);
	test_check("DSP boot flag clears before the EL71 timeout assertion", ready);
}
#endif

static struct boot_observation prepare_boot_observation(void) {
	struct boot_observation observation = { 0 };

	DSP_COM_CLEAR = 0xFFFF;
	SCU_DSP_INT = 0;
	for (uint32_t i = 0; i < BOOT_SAMPLE_LIMIT && (DSP_COM_STATUS & BOOT_FLAG) != 0; i++)
		if ((i & BOOT_WATCHDOG_INTERVAL_MASK) == BOOT_WATCHDOG_INTERVAL_MASK)
			test_watchdog_serve();

	dsp_hw_shared_memory[FW_VERSION_OFFSET] = FW_VERSION_SENTINEL;
	dsp_hw_shared_memory[HW_VERSION_OFFSET] = HW_VERSION_SENTINEL;
	observation.status_before_reset = DSP_COM_STATUS;
	return observation;
}

static struct boot_observation observe_boot_transition(struct boot_observation observation) {
	stopwatch_t start = stopwatch_get();

	for (uint32_t i = 0; i < BOOT_SAMPLE_LIMIT; i++) {
		uint32_t status = DSP_COM_STATUS;
		if (!observation.busy_seen && (status & BOOT_FLAG) != 0) {
			observation.busy_seen = true;
			observation.busy_iteration = i;
		}
		if (observation.busy_seen && (status & BOOT_FLAG) == 0) {
			observation.ready_seen = true;
			observation.ready_iteration = i;
			break;
		}

		if ((i & BOOT_WATCHDOG_INTERVAL_MASK) == BOOT_WATCHDOG_INTERVAL_MASK)
			test_watchdog_serve();
	}
	observation.elapsed_us = stopwatch_elapsed_us(start);
	observation.status_after_ready = DSP_COM_STATUS;
	return observation;
}

static struct boot_observation reset_and_observe(void) {
	struct boot_observation observation = prepare_boot_observation();

	SCU_RST_REQ = SCU_RST_REQ_DSP;
	(void) SCU_RST_REQ;
	SCU_RST_REQ = 0;
	return observe_boot_transition(observation);
}

static struct boot_observation clock_start_and_observe(void) {
	uint32_t cgu_con2 = CGU_CON2;

	CGU_CON2 = (cgu_con2 & ~CGU_CON2_DSP_CLKSEL) | CGU_CON2_DSP_CLKSEL_DISABLE;
	struct boot_observation observation = prepare_boot_observation();
	SCU_RST_REQ = SCU_RST_REQ_DSP;
	(void) SCU_RST_REQ;
	SCU_RST_REQ = 0;
	CGU_CON2 = cgu_con2;
	return observe_boot_transition(observation);
}

static bool wait_for_boot_command(void) {
	stopwatch_t start = stopwatch_get();
	while ((DSP_COM_STATUS & BOOT_FLAG) != 0 && stopwatch_elapsed_us(start) < BOOT_COMMAND_TIMEOUT_US)
		test_watchdog_serve();
	return (DSP_COM_STATUS & BOOT_FLAG) == 0;
}

static void validate_boot_start(size_t pass, const struct boot_observation *observation) {
	printf(
		"# DSP-BOOT,pass=%lu,busy=%u,ready=%u,busy_iter=%lu,ready_iter=%lu,elapsed_us=%lu,status=%08lX\n",
		(uint32_t) pass,
		observation->busy_seen,
		observation->ready_seen,
		observation->busy_iteration,
		observation->ready_iteration,
		observation->elapsed_us,
		observation->status_after_ready
	);

	test_eq_u32("communication flag is clear before DSP reset", 0, observation->status_before_reset & BOOT_FLAG);
	test_check("DSP autonomously raises communication flag zero after reset", observation->busy_seen);
	test_check("DSP autonomously clears communication flag zero after initialization", observation->ready_seen);
	test_eq_u32(
		"boot initialization leaves communication flag zero clear",
		0,
		observation->status_after_ready & BOOT_FLAG
	);
	test_eq_u32("boot initialization does not require an MCU interrupt", 0, SCU_DSP_INT);
	test_check("Mask ROM publishes a supported firmware version during autonomous initialization",
		mask_rom_is_supported(dsp_hw_shared_memory[FW_VERSION_OFFSET]));
	test_eq_u32(
		"Mask ROM publishes DSP subsystem version during autonomous initialization",
		DSP_SUBSYSTEM_VERSION,
		dsp_hw_shared_memory[HW_VERSION_OFFSET]
	);
}

static void test_clock_start(void) {
	test_category("Autonomous Mask ROM boot after DSP clock start");
	struct boot_observation observation = clock_start_and_observe();

	validate_boot_start(0, &observation);
}

static void test_boot_dispatcher_wait(void) {
	volatile uint16_t *boot_data = dsp_hw_shared_memory + BOOT_DATA_OFFSET;
	const uint16_t program_rom_fingerprint[] = {
		dsp_hw_shared_memory[FW_VERSION_OFFSET], 0xFFFF, 0x4180, PROGRAM_ROM_BRANCH_TARGET,
	};

	boot_data[0] = DSP_BOOT_PREAD;
	boot_data[1] = PROGRAM_ROM_FIRST;
	boot_data[2] = ARRAY_SIZE(program_rom_fingerprint);
	for (size_t i = 0; i < ARRAY_SIZE(program_rom_fingerprint); i++)
		boot_data[BOOT_RESULT_INDEX + i] = RESULT_SENTINEL;

	DSP_COM_SET = BOOT_FLAG;
	stopwatch_usleep_wd(BOOT_PENDING_OBSERVATION_US);
	test_eq_u32("boot command remains pending without an MCU interrupt", BOOT_FLAG, DSP_COM_STATUS & BOOT_FLAG);
	for (size_t i = 0; i < ARRAY_SIZE(program_rom_fingerprint); i++) {
		char name[72];

		sprintf(name, "boot result word %lu remains untouched before the interrupt", (uint32_t) i);
		test_eq_u32(name, RESULT_SENTINEL, boot_data[BOOT_RESULT_INDEX + i]);
	}
	test_eq_u32("test keeps the MCU-to-DSP interrupt deasserted while the command waits", 0, SCU_DSP_INT);

	SCU_DSP_INT = BOOT_FLAG;
	SCU_DSP_INT = 0;
	if (!test_check("MCU interrupt releases the pending boot command", wait_for_boot_command()))
		return;
	for (size_t i = 0; i < ARRAY_SIZE(program_rom_fingerprint); i++) {
		char name[64];

		sprintf(name, "PREAD returns Program Mask ROM word %lu", (uint32_t) i);
		test_eq_u32(name, program_rom_fingerprint[i], boot_data[BOOT_RESULT_INDEX + i]);
	}
}

static void run_pass(size_t pass) {
	char category[64];

	sprintf(category, "Autonomous Mask ROM boot after DSP reset / pass %lu", (uint32_t) pass);
	test_category(category);
	struct boot_observation observation = reset_and_observe();
	validate_boot_start(pass, &observation);
	if (!observation.ready_seen)
		return;

	test_boot_dispatcher_wait();
}

int main(void) {
	test_start("DSP boot-state functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_clock_start();
#ifdef PMB8876
	test_category("Firmware boot wait budget");
	test_el71_boot_wait_budget();
#endif

	for (size_t pass = 1; pass <= 2; pass++)
		run_pass(pass);

	DSP_COM_CLEAR = 0xFFFF;
	SCU_DSP_INT = 0;
	(void) dsp_hw_reset();
	return test_finish();
}
