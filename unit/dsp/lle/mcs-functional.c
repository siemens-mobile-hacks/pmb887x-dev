#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/dsp-hw.h"
#include "test.h"

#define READY_OFFSET 0x0300
#define DSP_INITIAL_VIEW_OFFSET 0x0301
#define DSP_ALTERNATING_VIEW_OFFSET 0x0302
#define DSP_OWNED_VIEW_OFFSET 0x0303
#define DSP_LOW_TRANSFER_VIEW_OFFSET 0x0304
#define DSP_COMPLETE_TRANSFER_VIEW_OFFSET 0x0305
#define DSP_RESET_VIEW_OFFSET 0x0306
#define READY_MARKER 0xA55A
#define COMPLETE_MARKER 0xA55A
#define SINGLE_SEMAPHORE BIT(5)
#define REQUEST_FLAG BIT(0)
#define RESPONSE_FLAG BIT(1)
#define EVEN_SEMAPHORE_MASK 0x5555
#define ODD_SEMAPHORE_MASK 0xAAAA
#define LOW_HALF_MASK 0x00FF
#define HIGH_HALF_MASK 0xFF00
#define ALL_SEMAPHORE_ENTRY_ADDRESS 0x0220

#ifdef PMB8875
#include "mcs-functional-8875.inc"
#define DSP_MCS_IMAGE DSP_MCS_IMAGE_8875
#else
#include "mcs-functional-8876.inc"
#define DSP_MCS_IMAGE DSP_MCS_IMAGE_8876
#endif

static bool wait_for_flags(uint32_t mask, uint32_t expected) {
	stopwatch_t start = stopwatch_get();

	while ((DSP_COM_STATUS & mask) != expected && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return (DSP_COM_STATUS & mask) == expected;
}

static bool wait_for_single_semaphore(uint32_t expected) {
	stopwatch_t start = stopwatch_get();

	while ((DSP_SEM_STATUS & SINGLE_SEMAPHORE) != expected && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return (DSP_SEM_STATUS & SINGLE_SEMAPHORE) == expected;
}

static bool wait_for_response(void) {
	stopwatch_t start = stopwatch_get();

	while ((DSP_COM_STATUS & RESPONSE_FLAG) == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return (DSP_COM_STATUS & RESPONSE_FLAG) != 0;
}

static bool execute_dsp_phase(const char *name) {
	DSP_COM_CLEAR = RESPONSE_FLAG;
	DSP_COM_SET = REQUEST_FLAG;
	bool completed = wait_for_response();

	test_check(name, completed);
	return completed;
}

static bool load_mcs_program(uint16_t entry_address) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	dsp_hw_shared_memory[READY_OFFSET] = 0;
	if (!test_check("boot commands load MCS test program", dsp_hw_load_image(DSP_MCS_IMAGE, sizeof(DSP_MCS_IMAGE))))
		return false;
	if (!test_check("BRANCH starts MCS test program", dsp_hw_branch(entry_address)))
		return false;
	return test_check("MCS test program becomes ready", dsp_hw_wait_shared(READY_OFFSET, READY_MARKER, 100));
}

static bool test_communication_flags(void) {
	test_category("MCS / Bidirectional communication flags");
	dsp_hw_shared_memory[0x0301] = 0x1357;
	DSP_COM_SET = BIT(1);
	if (!test_check("MCU request makes DSP set response flag", wait_for_flags(BIT(1) | BIT(2), BIT(2))))
		return false;
	test_eq_u32("DSP consumes MCU request flag", 0, DSP_COM_STATUS & BIT(1));
	test_eq_u32("DSP transforms MCU request payload", 0x1358, dsp_hw_shared_memory[0x0302]);
	DSP_COM_CLEAR = BIT(2);

	if (!test_check("DSP publishes inverse-direction request", wait_for_flags(BIT(3), BIT(3))))
		return false;
	test_eq_u32("DSP request payload is visible to MCU", 0x2468, dsp_hw_shared_memory[0x0304]);
	dsp_hw_shared_memory[0x0305] = 0x369C;
	DSP_COM_CLEAR = BIT(3);
	DSP_COM_SET = BIT(4);
	if (!test_check("DSP consumes MCU inverse response", dsp_hw_wait_shared(0x0303, 0x1111, 100)))
		return false;
	test_eq_u32("DSP transforms inverse response payload", 0x369D, dsp_hw_shared_memory[0x0302]);
	test_eq_u32("DSP clears inverse response flag", 0, DSP_COM_STATUS & BIT(4));

	return true;
}

static bool test_single_semaphore_transfer(void) {
	test_category("MCS / Single-semaphore ownership transfer");
	dsp_hw_shared_memory[0x0306] = 0;
	DSP_SEM_SET = SINGLE_SEMAPHORE;
	if (!test_check("MCU acquires free semaphore 5", wait_for_single_semaphore(0)))
		return false;
	DSP_COM_SET = BIT(5);
	stopwatch_usleep_wd(100);
	test_eq_u32("DSP sees semaphore busy while MCU owns it", UINT16_MAX, dsp_hw_shared_memory[0x0307]);
	test_eq_u32("DSP cannot update protected counter while MCU owns semaphore", 0, dsp_hw_shared_memory[0x0306]);
	test_eq_u32("DSP cannot publish completion while MCU owns semaphore", 0, DSP_COM_STATUS & BIT(6));
	DSP_SEM_CLEAR = SINGLE_SEMAPHORE;
	if (!test_check("MCU release transfers semaphore to waiting DSP", wait_for_flags(BIT(6), BIT(6))))
		return false;
	test_eq_u32("DSP reads ownership after MCU release", 0, dsp_hw_shared_memory[0x030C] & SINGLE_SEMAPHORE);
	test_eq_u32("DSP updates protected counter after ownership transfer", 1, dsp_hw_shared_memory[0x0306]);
	DSP_COM_CLEAR = BIT(6);
	if (!test_check("DSP release returns semaphore 5 to free state", wait_for_single_semaphore(SINGLE_SEMAPHORE)))
		return false;

	DSP_COM_SET = BIT(7);
	if (!test_check("DSP acquires free semaphore 5", dsp_hw_wait_shared(0x0308, 0xA1A1, 100)))
		return false;
	test_eq_u32("DSP reads zero after acquiring semaphore 5", 0,
		dsp_hw_shared_memory[0x0309] & SINGLE_SEMAPHORE);
	DSP_SEM_SET = SINGLE_SEMAPHORE;
	test_eq_u32("MCU sees semaphore busy while DSP owns it", SINGLE_SEMAPHORE,
		DSP_SEM_STATUS & SINGLE_SEMAPHORE);
	test_eq_u32("MCU cannot update protected counter while DSP owns semaphore", 1, dsp_hw_shared_memory[0x0306]);
	dsp_hw_shared_memory[0x030B] = 1;
	if (!test_check("DSP release transfers semaphore to waiting MCU", wait_for_single_semaphore(0)))
		return false;
	if (!test_check("DSP releases semaphore after protected update", wait_for_flags(BIT(8), BIT(8))))
		return false;
	test_eq_u32("DSP performs exactly one protected update", 2, dsp_hw_shared_memory[0x0306]);
	dsp_hw_shared_memory[0x0306]++;
	test_eq_u32("MCU performs one update after ownership transfer", 3, dsp_hw_shared_memory[0x0306]);
	DSP_SEM_CLEAR = SINGLE_SEMAPHORE;
	DSP_COM_CLEAR = BIT(8);
	return test_check("DSP reports semaphore scenario completion",
		dsp_hw_wait_shared(0x030A, COMPLETE_MARKER, 100));
}

static bool test_all_semaphore_arbitration(void) {
	test_category("MCS / All-semaphore arbitration");
	if (!load_mcs_program(ALL_SEMAPHORE_ENTRY_ADDRESS))
		return false;
	test_eq_u32("all 16 semaphores start in the free state", UINT16_MAX, DSP_SEM_STATUS);

	DSP_SEM_SET = UINT16_MAX;
	test_eq_u32("MCU atomically acquires all 16 free semaphores", 0, DSP_SEM_STATUS);
	if (!execute_dsp_phase("DSP queues requests for all MCU-owned semaphores"))
		return false;
	test_eq_u32("DSP non-owner release does not change MCU ownership", 0, DSP_SEM_STATUS);
	test_eq_u32("DSP reads all semaphores busy while MCU owns them",
		UINT16_MAX, dsp_hw_shared_memory[DSP_INITIAL_VIEW_OFFSET]);

	DSP_SEM_CLEAR = EVEN_SEMAPHORE_MASK;
	test_eq_u32("MCU release transfers every even semaphore waiter to DSP",
		EVEN_SEMAPHORE_MASK, DSP_SEM_STATUS);
	if (!execute_dsp_phase("DSP observes the even semaphore ownership transfer"))
		return false;
	test_eq_u32("DSP view is complementary to MCU after the even semaphore transfer",
		ODD_SEMAPHORE_MASK, dsp_hw_shared_memory[DSP_ALTERNATING_VIEW_OFFSET]);

	DSP_SEM_CLEAR = ODD_SEMAPHORE_MASK;
	test_eq_u32("second MCU release transfers all remaining waiters to DSP", UINT16_MAX, DSP_SEM_STATUS);
	if (!execute_dsp_phase("DSP re-request of owned semaphores is idempotent"))
		return false;
	test_eq_u32("DSP owns all semaphores after both transfers", 0,
		dsp_hw_shared_memory[DSP_OWNED_VIEW_OFFSET]);

	DSP_SEM_CLEAR = UINT16_MAX;
	test_eq_u32("MCU non-owner release does not change DSP ownership", UINT16_MAX, DSP_SEM_STATUS);
	DSP_SEM_SET = UINT16_MAX;
	test_eq_u32("MCU requests remain pending while DSP owns all semaphores", UINT16_MAX, DSP_SEM_STATUS);
	if (!execute_dsp_phase("DSP releases the low half to the waiting MCU"))
		return false;
	test_eq_u32("MCU owns the transferred low half", HIGH_HALF_MASK, DSP_SEM_STATUS);
	test_eq_u32("DSP sees the transferred low half as busy", LOW_HALF_MASK,
		dsp_hw_shared_memory[DSP_LOW_TRANSFER_VIEW_OFFSET]);
	if (!execute_dsp_phase("DSP releases the high half to the waiting MCU"))
		return false;
	test_eq_u32("MCU owns all semaphores after the complete transfer", 0, DSP_SEM_STATUS);
	test_eq_u32("DSP sees all semaphores busy after the complete transfer", UINT16_MAX,
		dsp_hw_shared_memory[DSP_COMPLETE_TRANSFER_VIEW_OFFSET]);

	DSP_SEM_SET = UINT16_MAX;
	test_eq_u32("MCU re-request of owned semaphores is idempotent", 0, DSP_SEM_STATUS);
	DSP_SEM_CLEAR = EVEN_SEMAPHORE_MASK;
	test_eq_u32("owner release without a waiter returns semaphores to free state",
		EVEN_SEMAPHORE_MASK, DSP_SEM_STATUS);
	DSP_SEM_CLEAR = EVEN_SEMAPHORE_MASK;
	test_eq_u32("releasing already-free semaphores has no effect", EVEN_SEMAPHORE_MASK, DSP_SEM_STATUS);
	DSP_SEM_CLEAR = ODD_SEMAPHORE_MASK;
	test_eq_u32("releasing the remaining owner bits frees all semaphores", UINT16_MAX, DSP_SEM_STATUS);

	DSP_SEM_SET = UINT16_MAX;
	if (!execute_dsp_phase("DSP queues semaphore requests before reset"))
		return false;
	test_eq_u32("MCU still owns all semaphores with a pending DSP waiter", 0, DSP_SEM_STATUS);
	if (!test_check("DSP reset completes with pending semaphore waiters", dsp_hw_reset()))
		return false;
	test_eq_u32("DSP reset preserves MCU semaphore ownership", 0, DSP_SEM_STATUS);
	DSP_SEM_CLEAR = UINT16_MAX;
	test_eq_u32("MCU release after DSP reset returns all semaphores to free state", UINT16_MAX, DSP_SEM_STATUS);
	dsp_hw_shared_memory[READY_OFFSET] = 0;
	if (!test_check("semaphore program reloads after reset",
		dsp_hw_load_image(DSP_MCS_IMAGE, sizeof(DSP_MCS_IMAGE)))) {
		return false;
	}
	if (!test_check("reloaded semaphore program starts", dsp_hw_branch(ALL_SEMAPHORE_ENTRY_ADDRESS)))
		return false;
	if (!test_check("reloaded semaphore program becomes ready",
		dsp_hw_wait_shared(READY_OFFSET, READY_MARKER, 100))) {
		return false;
	}
	test_eq_u32("DSP reset discards its pending waiter without releasing MCU ownership",
		UINT16_MAX, dsp_hw_shared_memory[DSP_RESET_VIEW_OFFSET]);
	test_eq_u32("semaphores remain free after the reset probe", UINT16_MAX, DSP_SEM_STATUS);

	return true;
}

int main(void) {
	test_start("DSP MCS functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!load_mcs_program(DSP_HW_STARTUP_ADDRESS))
		return test_finish();
	if (!test_communication_flags())
		return test_finish();
	if (!test_single_semaphore_transfer())
		return test_finish();
	if (!test_all_semaphore_arbitration())
		return test_finish();

	return test_finish();
}
