#include <pmb887x.h>
#include <gen/dsp.h>
#include <wdt.h>

#include "dsp-hw.h"
#include "test.h"
#include "tpu-events-8876.inc"

#define TPU_TIMER_RAM_BASE 512
#define TPU_DECODER_SHIFT 6
#define TPU_EVENT_TICK 100
#define TPU_FRAME_TICKS 1000
#define TPU_TIMEOUT_MS 100
#define READY_MARKER 0xA55A
#define READY_OFFSET 0x000F
#define SNAPSHOT_REQUEST_OFFSET 0x0010
#define SNAPSHOT_COMPLETE_OFFSET 0x0011
#define FINTA0_OFFSET 0x0020
#define FINTB0_OFFSET 0x0021
#define BB_STATUS_OFFSET 0x0022
#define MOD_STATUS_OFFSET 0x0023
#define BB_RECEIVE_STATUS_MASK \
	(TEAK_BB_STATUS_EQON | TEAK_BB_STATUS_FCON | TEAK_BB_STATUS_MONON | TEAK_BB_STATUS_SCON)
#define BB_EDGE_FLAGS (TEAK_INT_FINTA0_BBHI | TEAK_INT_FINTA0_BBLO)
#define TPU_DECODER_FINTA_FLAGS \
	(BB_EDGE_FLAGS | TEAK_INT_FINTA0_FRAME | TEAK_INT_FINTA0_CODONHI | TEAK_INT_FINTA0_CODONLO)

struct dsp_snapshot {
	uint16_t finta0;
	uint16_t fintb0;
	uint16_t bb_status;
	uint16_t mod_status;
};

struct receive_event {
	const char *name;
	uint16_t decoder;
	uint16_t status;
};

static const struct receive_event RECEIVE_SET_EVENTS[] = {
	{ "EQON", 1, TEAK_BB_STATUS_EQON },
	{ "MONON", 2, TEAK_BB_STATUS_MONON },
	{ "SCON", 3, TEAK_BB_STATUS_SCON },
	{ "FCON", 4, TEAK_BB_STATUS_FCON },
};

static uint16_t snapshot_sequence;

static bool wait_event_pointer(uint32_t value) {
	stopwatch_t start = stopwatch_get();

	while (TPU_CEAP != value && stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS)
		test_watchdog_serve();

	return TPU_CEAP == value;
}

static void configure_events(const uint16_t *decoders, size_t count) {
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
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3 + 1) = TPU_EVENT_TICK + i * 20;
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3 + 2) = decoders[i] << TPU_DECODER_SHIFT;
	}
	TPU_RAM(TPU_TIMER_RAM_BASE + count * 3) = 0;
	TPU_RAM(TPU_TIMER_RAM_BASE + count * 3 + 1) = TPU_EVENT_TICK + count * 20 + 100;
	TPU_RAM(TPU_TIMER_RAM_BASE + count * 3 + 2) = 0;
}

static bool run_events(const uint16_t *decoders, size_t count) {
	configure_events(decoders, count);
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	bool initialized = wait_event_pointer(0);
	if (!test_check("TINI restarts the Timer RAM event pointer at EAPB", initialized)) {
		TPU_PARAM = 0;
		return false;
	}
	bool completed = wait_event_pointer(count * 3);
	stopwatch_usleep_wd(100);
	TPU_PARAM = 0;

	if (!test_check("event engine reaches the trailing no-action Timer RAM event", completed))
		return false;
	return true;
}

static bool take_snapshot(struct dsp_snapshot *snapshot) {
	snapshot_sequence++;
	if (snapshot_sequence == 0)
		snapshot_sequence++;
	dsp_hw_shared_memory[SNAPSHOT_REQUEST_OFFSET] = snapshot_sequence;
	bool completed = dsp_hw_wait_shared(SNAPSHOT_COMPLETE_OFFSET, snapshot_sequence, TPU_TIMEOUT_MS);
	if (!test_check("DSP records decoder-visible state", completed))
		return false;

	snapshot->finta0 = dsp_hw_shared_memory[FINTA0_OFFSET];
	snapshot->fintb0 = dsp_hw_shared_memory[FINTB0_OFFSET];
	snapshot->bb_status = dsp_hw_shared_memory[BB_STATUS_OFFSET];
	snapshot->mod_status = dsp_hw_shared_memory[MOD_STATUS_OFFSET];
	return true;
}

static bool clear_decoder_state(void) {
	static const uint16_t CLEAR_EVENTS[] = { 5, 7, 9, 18 };
	struct dsp_snapshot snapshot;

	if (!run_events(CLEAR_EVENTS, ARRAY_SIZE(CLEAR_EVENTS)))
		return false;
	if (!take_snapshot(&snapshot))
		return false;
	test_eq_u32("receive decoder state starts clear", 0, snapshot.bb_status & BB_RECEIVE_STATUS_MASK);
	test_eq_u32("RXON decoder state starts clear", 0, snapshot.bb_status & TEAK_BB_STATUS_RXON);
	test_eq_u32("CODON decoder state starts inactive", 0, snapshot.mod_status & TEAK_MOD_STAT_MSTAT);
	return true;
}

static void test_no_action_value(uint16_t decoder) {
	static const uint16_t ACTIVE_EVENTS[] = { 1, 2, 3, 4, 6, 17 };
	struct dsp_snapshot snapshot;
	char category[40];

	tfp_sprintf(category, "Decoder %u performs no action", (uint32_t) decoder);
	test_category(category);
	if (!clear_decoder_state() || !run_events(ACTIVE_EVENTS, ARRAY_SIZE(ACTIVE_EVENTS)) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("precondition activates all observable Baseband signals",
		BB_RECEIVE_STATUS_MASK | TEAK_BB_STATUS_RXON,
		snapshot.bb_status & (BB_RECEIVE_STATUS_MASK | TEAK_BB_STATUS_RXON));
	test_eq_u32("precondition activates CODON", TEAK_MOD_STAT_MSTAT, snapshot.mod_status & TEAK_MOD_STAT_MSTAT);
	if (!run_events(&decoder, 1) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("no-action decoder preserves all observable Baseband signals",
		BB_RECEIVE_STATUS_MASK | TEAK_BB_STATUS_RXON,
		snapshot.bb_status & (BB_RECEIVE_STATUS_MASK | TEAK_BB_STATUS_RXON));
	test_eq_u32("no-action decoder preserves CODON", TEAK_MOD_STAT_MSTAT,
		snapshot.mod_status & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("no-action decoder raises no decoder-driven INT0 A flags", 0,
		snapshot.finta0 & TPU_DECODER_FINTA_FLAGS);
	test_eq_u32("no-action decoder raises no INT0 B flags", 0, snapshot.fintb0);
}

static void test_no_action(void) {
	static const uint16_t VALUES[] = { 0, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };

	for (size_t i = 0; i < ARRAY_SIZE(VALUES); i++)
		test_no_action_value(VALUES[i]);
}

static void test_receive_set_events(void) {
	for (size_t i = 0; i < ARRAY_SIZE(RECEIVE_SET_EVENTS); i++) {
		const struct receive_event *event = &RECEIVE_SET_EVENTS[i];
		struct dsp_snapshot snapshot;
		char category[48];
		uint16_t decoder = event->decoder;

		tfp_sprintf(category, "Decoder %u sets %s", (uint32_t) event->decoder, event->name);
		test_category(category);
		if (!clear_decoder_state() || !run_events(&decoder, 1) || !take_snapshot(&snapshot))
			continue;
		test_eq_u32("selected receive signal becomes active", event->status,
			snapshot.bb_status & BB_RECEIVE_STATUS_MASK);
		test_eq_u32("receive rising edge raises only BBHI", TEAK_INT_FINTA0_BBHI,
			snapshot.finta0 & BB_EDGE_FLAGS);
	}
}

static void test_receive_clear(void) {
	static const uint16_t SET_EVENTS[] = { 1, 2, 3, 4 };
	uint16_t clear_event = 5;
	struct dsp_snapshot snapshot;

	test_category("Decoder 5 clears all receive modes");
	if (!clear_decoder_state() || !run_events(SET_EVENTS, ARRAY_SIZE(SET_EVENTS)) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("all four receive modes can be active together", BB_RECEIVE_STATUS_MASK,
		snapshot.bb_status & BB_RECEIVE_STATUS_MASK);
	if (!run_events(&clear_event, 1) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("decoder 5 clears EQON, MONON, SCON, and FCON", 0,
		snapshot.bb_status & BB_RECEIVE_STATUS_MASK);
	test_eq_u32("receive falling edges raise only BBLO", TEAK_INT_FINTA0_BBLO,
		snapshot.finta0 & BB_EDGE_FLAGS);
}

static void test_repeated_monon(void) {
	uint16_t monon_event = 2;
	struct dsp_snapshot snapshot;

	test_category("Repeated decoder 2 MONON set");
	if (!clear_decoder_state() || !run_events(&monon_event, 1) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("first MONON set raises BBHI", TEAK_INT_FINTA0_BBHI, snapshot.finta0 & BB_EDGE_FLAGS);
	if (!run_events(&monon_event, 1) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("repeated MONON set preserves active status", TEAK_BB_STATUS_MONON,
		snapshot.bb_status & TEAK_BB_STATUS_MONON);
	test_eq_u32("repeated MONON set does not create another edge", 0, snapshot.finta0 & BB_EDGE_FLAGS);
}

static void test_rxon(void) {
	uint16_t event = 6;
	struct dsp_snapshot snapshot;

	test_category("Decoders 6 and 7 control RXON");
	if (!clear_decoder_state() || !run_events(&event, 1) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("decoder 6 sets RXON", TEAK_BB_STATUS_RXON, snapshot.bb_status & TEAK_BB_STATUS_RXON);
	test_eq_u32("RXON rising edge does not raise Baseband edge IRQ", 0, snapshot.finta0 & BB_EDGE_FLAGS);
	event = 7;
	if (!run_events(&event, 1) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("decoder 7 clears RXON", 0, snapshot.bb_status & TEAK_BB_STATUS_RXON);
	test_eq_u32("RXON falling edge does not raise Baseband edge IRQ", 0, snapshot.finta0 & BB_EDGE_FLAGS);
}

static void test_txon(void) {
	static const uint16_t EVENTS[] = { 8, 9 };
	struct dsp_snapshot snapshot;

	test_category("Decoders 8 and 9 control analog TXON");
	if (!clear_decoder_state() || !run_events(EVENTS, ARRAY_SIZE(EVENTS)) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("TXON events do not alter digital modulator status", 0,
		snapshot.mod_status & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("TXON events raise no DSP INT0 A flags", 0, snapshot.finta0);
	test_eq_u32("TXON events raise no DSP INT0 B flags", 0, snapshot.fintb0);
}

static void test_toggle_interrupt(uint16_t decoder, uint16_t a_mask, uint16_t b_mask, const char *category) {
	struct dsp_snapshot snapshot;

	test_category(category);
	if (!clear_decoder_state() || !run_events(&decoder, 1) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("first toggle raises the expected INT0 A flag", a_mask, snapshot.finta0);
	test_eq_u32("first toggle raises the expected INT0 B flag", b_mask, snapshot.fintb0);
	if (!run_events(&decoder, 1) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("second toggle raises the same INT0 A flag", a_mask, snapshot.finta0);
	test_eq_u32("second toggle raises the same INT0 B flag", b_mask, snapshot.fintb0);
}

static void test_codon(void) {
	uint16_t event = 17;
	struct dsp_snapshot snapshot;

	test_category("Decoders 17 and 18 control CODON");
	if (!clear_decoder_state() || !run_events(&event, 1) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("decoder 17 raises CODONHI", TEAK_INT_FINTA0_CODONHI,
		snapshot.finta0 & (TEAK_INT_FINTA0_CODONHI | TEAK_INT_FINTA0_CODONLO));
	test_eq_u32("decoder 17 activates the digital modulator", TEAK_MOD_STAT_MSTAT,
		snapshot.mod_status & TEAK_MOD_STAT_MSTAT);
	event = 18;
	if (!run_events(&event, 1) || !take_snapshot(&snapshot))
		return;
	test_eq_u32("decoder 18 raises CODONLO", TEAK_INT_FINTA0_CODONLO,
		snapshot.finta0 & (TEAK_INT_FINTA0_CODONHI | TEAK_INT_FINTA0_CODONLO));
	test_eq_u32("decoder 18 stops the digital modulator", 0, snapshot.mod_status & TEAK_MOD_STAT_MSTAT);
}

static bool prepare_runner(void) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = 0xFFFF;
	bool loaded = dsp_hw_load_image(DSP_TPU_EVENTS_8876, sizeof(DSP_TPU_EVENTS_8876));
	if (!test_check("boot commands load the TPU event observer", loaded))
		return false;
	if (!test_check("BRANCH starts the TPU event observer", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	return test_check("TPU event observer becomes ready", dsp_hw_wait_shared(READY_OFFSET, READY_MARKER, TPU_TIMEOUT_MS));
}

int main(void) {
	test_start("TPU Timer RAM decoder event test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	wdt_set_max_execution_time(UINT32_MAX);

	if (prepare_runner()) {
		test_no_action();
		test_receive_set_events();
		test_receive_clear();
		test_repeated_monon();
		test_rxon();
		test_txon();
		test_toggle_interrupt(15, TEAK_INT_FINTA0_FRAME, 0, "Decoder 15 toggles FRAME");
		test_toggle_interrupt(16, 0, TEAK_INT_FINTB0_SYSMCU, "Decoder 16 toggles SYSMCU");
		test_codon();
	}

	TPU_PARAM = 0;
	DSP_COM_CLEAR = 0xFFFF;
	(void) dsp_hw_reset();
	return test_finish();
}
