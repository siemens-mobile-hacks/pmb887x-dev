#include <pmb887x.h>

#include "test.h"

#define DSP_BOOT_DREAD 4
#define DSP_BOOT_DATA_OFFSET 2
#define DSP_BOOT_RESULT_OFFSET (DSP_BOOT_DATA_OFFSET + 3)
#define DSP_WAIT_ITERATIONS 1000000
#define DSP_PERIPHERAL_FIRST 0xDE00
#define DSP_PERIPHERAL_LAST 0xDEFF
#define DSP_PROBE_ATTEMPTS 2

static volatile uint16_t *const DSP_SHARED_MEMORY = (volatile uint16_t *) DSP_RAM_BASE;

struct probe_result {
	bool reset_completed;
	bool read_completed;
	uint16_t value;
};

static bool wait_for_boot_ready(void) {
	for (size_t i = 0; i < DSP_WAIT_ITERATIONS; i++) {
		if ((DSP_COM_STATUS & BIT(0)) == 0)
			return true;
		if ((i & 0x3FFF) == 0)
			test_watchdog_serve();
	}

	return false;
}

static bool reset_dsp(void) {
	DSP_COM_CLEAR = UINT16_MAX;
	SCU_DSP_INT = 0;
	SCU_RST_REQ = SCU_RST_REQ_DSP;
	SCU_RST_REQ = 0;

	return wait_for_boot_ready();
}

static bool read_data_word(uint16_t address, uint16_t *value) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = DSP_BOOT_DREAD;
	boot_data[1] = address;
	boot_data[2] = 1;
	DSP_COM_SET = BIT(0);
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;
	if (!wait_for_boot_ready())
		return false;

	*value = DSP_SHARED_MEMORY[DSP_BOOT_RESULT_OFFSET];
	return true;
}

static struct probe_result probe_address(uint16_t address) {
	struct probe_result result = { 0 };

	result.reset_completed = reset_dsp();
	if (result.reset_completed)
		result.read_completed = read_data_word(address, &result.value);

	return result;
}

static const char *probe_status(const struct probe_result *result) {
	if (!result->reset_completed)
		return "RESET_TIMEOUT";
	if (!result->read_completed)
		return "READ_TIMEOUT";

	return "OK";
}

int main(void) {
	test_start("DSP data-space peripheral read map");

	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	if (!test_check("DSP Mask ROM boot dispatcher becomes ready", reset_dsp()))
		return test_finish();

	uint16_t mask_id = DSP_SHARED_MEMORY[0];
	printf("# transport=mask-rom DREAD mask-id=%04X range=%04X-%04X attempts=%u reset-before-each=yes\n",
		(uint32_t) mask_id, (uint32_t) DSP_PERIPHERAL_FIRST, (uint32_t) DSP_PERIPHERAL_LAST,
		(uint32_t) DSP_PROBE_ATTEMPTS);
	printf("# DSPDATA,address,status1,value1,status2,value2,classification\n");

	size_t completed = 0;
	size_t stable = 0;
	size_t unstable = 0;
	size_t read_timeouts = 0;
	size_t reset_timeouts = 0;
	for (uint32_t address = DSP_PERIPHERAL_FIRST; address <= DSP_PERIPHERAL_LAST; address++) {
		struct probe_result results[DSP_PROBE_ATTEMPTS];

		for (size_t attempt = 0; attempt < DSP_PROBE_ATTEMPTS; attempt++) {
			results[attempt] = probe_address((uint16_t) address);
			if (!results[attempt].reset_completed) {
				reset_timeouts++;
			} else if (!results[attempt].read_completed) {
				read_timeouts++;
			} else {
				completed++;
			}
		}

		const char *classification;
		if (results[0].read_completed && results[1].read_completed) {
			if (results[0].value == results[1].value) {
				classification = "STABLE";
				stable++;
			} else {
				classification = "UNSTABLE";
				unstable++;
			}
		} else {
			classification = "INCOMPLETE";
		}

		printf("# DSPDATA,%04X,%s,%04X,%s,%04X,%s\n", address,
			probe_status(&results[0]), results[0].value, probe_status(&results[1]), results[1].value,
			classification);
		test_watchdog_serve();
	}

	printf("# summary: completed=%u stable=%u unstable=%u read-timeouts=%u reset-timeouts=%u\n",
		(uint32_t) completed, (uint32_t) stable, (uint32_t) unstable,
		(uint32_t) read_timeouts, (uint32_t) reset_timeouts);
	test_check("DSP reset recovers every isolated probe", reset_timeouts == 0);
	test_check("at least one peripheral-window read completes", completed != 0);

	return test_finish();
}
