#include <pmb887x.h>
#include <gen/dsp.h>
#include <stopwatch.h>
#include <wdt.h>

#include "board/gsm.h"
#include "dsp-hw.h"
#include "gsm.h"
#include "gsm-trx.h"
#include "pmb8876-firmware-0801.inc"
#include "test.h"

#define TPU_TIMER_RAM_BASE 512
#define TPU_TIMER_BANK_0_OFFSET 0
#define TPU_TIMER_BANK_1_OFFSET 0xFF
#define TPU_FRAME_TICKS 10000
#define TPU_FRAME_INTERRUPT_TICK 0x2328
/* Stock transition: four masked search frames, the initialization prefix, then the first search frame. */
#define TPU_RF_QUIET_FRAME_COUNT 4
#define TPU_RF_FIRST_QUIET_EVENT_GROUPS 0x8000C000
#define TPU_RF_QUIET_EVENT_GROUPS 0x80004000
#define TPU_RF_INITIALIZATION_PREFIX_EVENT_GROUPS 0x8000400A
#define TPU_FIRST_SEARCH_EVENT_GROUPS 0x80004003
#define TPU_SEARCH_EVENT_GROUPS 0x8000C003
#define TPU_RF_POWER_UP_EVENT_GROUPS 0x8000C000
#define TEST_TIMEOUT_MS 100
#define SCAN_TIMEOUT_MS 60000
#define FCCH_TIMEOUT_MS 15000
#define GSM_CHANNEL_RASTER_HZ 200000
#define RUNTIME_PIPE_OFFSET 0x0005
#define DSP_COMMAND_MODU_INIT 2
#define DSP_COMMAND_FC_INIT 1
#define DSP_COMMAND_IQ_SWAP_1 3
#define DSP_COMMAND_IQ_SWAP_2 4
#define DSP_COMMAND_DEC_INIT 5
#define DSP_COMMAND_BB_OFF 13
#define DSP_COMMAND_IDLE 14
#define DSP_COMMAND_RF_ADAPT 67
#define RF_ADAPT_EXTENSION_OFFSET 0x18
#define RF_ADAPT_EXTENSION_VALUE 0xFB8C
#define BB_OFF_SEQUENCE_OFFSET 0x500
#define DSP_RUNTIME_CONFIG_OFFSET 0x57A
#define DSP_RUNTIME_CONFIG_VALUE 0xC000
#define MON_INDEX_OFFSET 191
#define MON_VALUES_OFFSET 192
#define MON_VALUE_COUNT 8
#define MON_RESULT_FIRST 4
#define MON_RAW_VALUES_PER_DB 16
#define FC_STATUS_OFFSET 89
#define FC_START_OFFSET 90
#define FC_QUALITY_OFFSET 91
#define FC_RMS_OFFSET 92
#define FC_FREQUENCY_OFFSET 93
#define FC_RESULT_WORDS 5
#define FC_SENTINEL_BASE 0xA500
#define FC_COMMUNICATION_FLAGS (BIT(3) | BIT(4))
/* Keep the FCCH search bounded while trying the strongest monitored channels first. */
#define FCCH_CANDIDATE_COUNT 16
/* The stock search schedules five frequency-correction windows before advancing. */
#define FCCH_ATTEMPT_COUNT 5
#define TPU_FCCH_PREPARE_EVENT_GROUPS 0x8000C041
#define TPU_FCCH_START_EVENT_GROUPS 0x8000C003
#define TPU_FCCH_HOLD_EVENT_GROUPS 0x8000C001
#define SCAN_PASS_COUNT 16
#define SCAN_WINDOW_STRIDE 8
#define GSM_SCAN_CHANNEL_COUNT (GSM_CHANNEL_COUNT - 1)
#define TPU_RF_TELEGRAM_TRIGGER 8
#define TPU_RF_TRIGGER_MASK 0x3F
#define TPU_INITIALIZATION_EVENT_GROUPS 0x8000C00A

struct tpu_event {
	uint16_t control;
	uint16_t time;
	uint16_t action;
};

enum tpu_loop_phase {
	TPU_LOOP_RF_POWER_UP,
	TPU_LOOP_RF_INITIALIZATION,
	TPU_LOOP_READY,
	TPU_LOOP_SEARCH,
	TPU_LOOP_FCCH,
	TPU_LOOP_COMPLETE,
};

enum receiver_scan_state {
	RECEIVER_SCAN_IDLE,
	RECEIVER_SCAN_REQUESTED,
	RECEIVER_SCAN_ACTIVE,
	RECEIVER_SCAN_COMPLETE,
};

enum fcch_scan_state {
	FCCH_SCAN_IDLE,
	FCCH_SCAN_REQUESTED,
	FCCH_SCAN_BB_OFF_REQUESTED,
	FCCH_SCAN_COMMAND_REQUESTED,
	FCCH_SCAN_ACTIVE,
	FCCH_SCAN_COMPLETE,
};

enum fcch_tpu_phase {
	FCCH_TPU_RF_QUIET_FIRST,
	FCCH_TPU_RF_QUIET_SECOND,
	FCCH_TPU_RF_INITIALIZATION,
	FCCH_TPU_PREPARE_FIRST,
	FCCH_TPU_PREPARE_SECOND,
	FCCH_TPU_START,
	FCCH_TPU_HOLD_FIRST,
	FCCH_TPU_HOLD_SECOND,
	FCCH_TPU_FINISH,
};

struct gsm_scan_range {
	const char *name;
	enum gsm_band band;
	uint16_t table_first;
	uint16_t arfcn_first;
	uint16_t channel_count;
	uint32_t frequency_first;
};

struct fcch_result {
	uint16_t status;
	uint16_t start;
	uint16_t quality;
	int16_t rms;
	int16_t frequency;
	uint16_t communication_flags;
};

struct fcch_candidate {
	uint16_t channel_index;
	uint16_t arfcn;
	int16_t level_dbm_x16;
};

struct channel_scan_result {
	int16_t level_dbm_x16;
	uint32_t measurements;
	uint16_t gain_state;
};

struct scan_stats {
	uint32_t channels;
	uint32_t valid_channels;
	uint32_t monitor_samples;
	uint32_t different_channels;
	uint32_t minimum_frequency;
	uint32_t maximum_frequency;
	int16_t baseline;
	int16_t minimum;
	int16_t maximum;
};

static const struct gsm_scan_range GSM_SCAN_RANGES[] = {
	{ "GSM900", GSM_BAND_900, 0, 0, 125, 935000000 },
	{ "GSM900", GSM_BAND_900, 126, 975, 49, 925200000 },
	{ "DCS1800", GSM_BAND_DCS1800, 175, 512, 374, 1805200000 },
	{ "PCS1900", GSM_BAND_PCS1900, 549, 512, 299, 1930200000 },
	{ "GSM850", GSM_BAND_850, 848, 128, 124, 869200000 },
};

/* TPU RF initialization window captured from firmware 0801 in QEMU. */
static const struct tpu_event RF_INITIALIZATION_EVENTS[] = {
	{ 0x4000, 0x0001, 0x03C0 },
	{ 0x8600, 0x0004, 0x0140 },
	{ 0x8600, 0x0190, 0xC000 },
	{ 0x4600, 0x07D0, 0x0008 },
	{ 0x4600, 0x0834, 0x0009 },
	{ 0x4600, 0x0898, 0x000A },
	{ 0x4600, 0x08FC, 0x000B },
	{ 0x4600, 0x0960, 0x000C },
	{ 0x4600, 0x09C4, 0x000D },
	{ 0x4600, 0x0A28, 0x000E },
	{ 0x4600, 0x0A8C, 0x000F },
	{ 0x4600, 0x0AF0, 0x0010 },
	{ 0x4600, 0x0B54, 0x0011 },
	{ 0x7E00, 0x7FF8, 0x1000 },
};

/* Four-monitor search window captured from firmware 0801 in QEMU. */
static const struct tpu_event SIGNAL_SEARCH_EVENTS[] = {
	{ 0x4000, 0x0001, 0x03C0 },
	{ 0x4200, 0x0A10, 0x0008 },
	{ 0x4200, 0x0ABE, 0x000C },
	{ 0x4200, 0x0B92, 0x0180 },
	{ 0x4200, 0x0C2A, 0x0080 },
	{ 0x8200, 0x0E3A, 0x0155 },
	{ 0x8200, 0x0E3C, 0x01C0 },
	{ 0x4200, 0x1000, 0x0009 },
	{ 0x4200, 0x10AE, 0x000D },
	{ 0x4200, 0x1182, 0x0180 },
	{ 0x4200, 0x121A, 0x0080 },
	{ 0x8200, 0x142A, 0x0155 },
	{ 0x8200, 0x142C, 0x01C0 },
	{ 0x4200, 0x15F0, 0x000A },
	{ 0x4200, 0x169E, 0x000E },
	{ 0x4200, 0x1772, 0x0180 },
	{ 0x4200, 0x180A, 0x0080 },
	{ 0x8200, 0x1A1A, 0x0155 },
	{ 0x8200, 0x1A1C, 0x01C0 },
	{ 0x5F00, 0x1A40, 0x0000 },
	{ 0x5E00, 0x1AFE, 0x0800 },
	{ 0x9F00, 0x1B08, 0x0000 },
	{ 0x5E00, 0x1BC6, 0x0280 },
	{ 0x4200, 0x1BE0, 0x000B },
	{ 0x4200, 0x1C8E, 0x000F },
	{ 0x4200, 0x1D62, 0x0180 },
	{ 0x4200, 0x1DFA, 0x0080 },
	{ 0x8200, 0x200A, 0x0155 },
	{ 0x8200, 0x200C, 0x01C0 },
	{ 0x7E00, 0x7FF8, 0x1000 },
};

/* Multi-frame frequency-correction search window captured from firmware 0801. */
static const struct tpu_event FCCH_EVENTS[] = {
	{ 0x4000, 0x0001, 0x03C0 },
	{ 0x8C00, 0x0008, 0x0155 },
	{ 0x8C00, 0x000A, 0x01C0 },
	{ 0x4200, 0x003C, 0x000B },
	{ 0x4200, 0x00EA, 0x000A },
	{ 0x4200, 0x01BE, 0x0180 },
	{ 0x4200, 0x0256, 0x0100 },
	{ 0x8800, 0x1120, 0x0155 },
	{ 0x8800, 0x1122, 0x01C0 },
	{ 0x9000, 0x2E6C, 0x0155 },
	{ 0x9000, 0x2E6E, 0x01C0 },
	{ 0x7E00, 0x7FF8, 0x1000 },
};

static const struct tpu_event TPU_IDLE_EVENTS[] = {
	{ 0x4000, 0x0001, 0x03C0 },
	{ 0x7E00, 0x7FF8, 0x1000 },
};

static const struct tpu_event RF_POWER_UP_EVENTS[] = {
	{ 0xFE00, 0x0000, 0x0000 },
};

static volatile enum tpu_loop_phase tpu_loop_phase;
static volatile bool rf_initialization_complete;
static volatile enum receiver_scan_state receiver_scan_state;
static volatile enum fcch_scan_state fcch_scan_state;
static uint32_t scan_pass;
static int scan_window_start;
static bool scan_request_repeated;
static uint32_t tpu_schedule_bank;
static uint32_t tpu_rf_transition_frame;
static bool tpu_loop_running;
static bool dsp_clock_enabled;
static uint16_t scan_channels[GSM_SCAN_CHANNEL_COUNT];
static uint16_t scan_request_channels[GSM_TRX_MONITOR_COUNT];
static uint16_t scan_request_gain_states[GSM_TRX_MONITOR_COUNT];
static struct channel_scan_result scan_results[GSM_CHANNEL_COUNT];
static struct fcch_candidate fcch_candidates[FCCH_CANDIDATE_COUNT];
static struct fcch_result fcch_results[FCCH_CANDIDATE_COUNT][FCCH_ATTEMPT_COUNT];
static uint32_t fcch_candidate_attempts[FCCH_CANDIDATE_COUNT];
static uint32_t fcch_candidate_count;
static uint32_t fcch_candidate_index;
static uint32_t fcch_window;
static uint32_t fcch_candidate_detections;
static int32_t fcch_candidate_frequency_sum;
static uint16_t fcch_channel_index;
static volatile enum fcch_tpu_phase fcch_tpu_phase;

static void clear_tpu_ram(void) {
	for (size_t i = 0; i < TPU_RAM_SIZE / sizeof(uint32_t); i++)
		TPU_RAM(i) = 0;
}

static void write_tpu_schedule(const struct tpu_event *events, size_t count, uint32_t bank) {
	uint32_t event_base = bank == 0 ? TPU_TIMER_BANK_0_OFFSET : TPU_TIMER_BANK_1_OFFSET;

	for (size_t i = 0; i < count; i++) {
		uint16_t action = events[i].action;
		uint16_t rf_trigger = action & TPU_RF_TRIGGER_MASK;
		bool is_rf_event = rf_trigger >= TPU_RF_TELEGRAM_TRIGGER &&
			rf_trigger < TPU_RF_TELEGRAM_TRIGGER + GSM_TRX_TELEGRAM_COUNT;
		if (is_rf_event)
			action += bank * GSM_TRX_TELEGRAM_COUNT;

		TPU_RAM(TPU_TIMER_RAM_BASE + event_base + i * 3) = events[i].control;
		TPU_RAM(TPU_TIMER_RAM_BASE + event_base + i * 3 + 1) = events[i].time;
		TPU_RAM(TPU_TIMER_RAM_BASE + event_base + i * 3 + 2) = action;
	}
}

static void write_initial_tpu_schedules(void) {
	write_tpu_schedule(RF_POWER_UP_EVENTS, ARRAY_SIZE(RF_POWER_UP_EVENTS), 0);
	write_tpu_schedule(RF_INITIALIZATION_EVENTS, ARRAY_SIZE(RF_INITIALIZATION_EVENTS), 1);
}

static void publish_tpu_event_window(uint32_t bank, size_t event_count, uint32_t event_groups) {
	uint32_t first_event = bank == 0 ? TPU_TIMER_BANK_0_OFFSET : TPU_TIMER_BANK_1_OFFSET;

	TPU_EAPT = first_event + event_count * 3;
	TPU_EAPB = first_event;
	TPU_TGER = event_groups;
}

static void publish_rf_initialization_frame(uint32_t bank, uint32_t event_groups) {
	write_tpu_schedule(RF_INITIALIZATION_EVENTS, ARRAY_SIZE(RF_INITIALIZATION_EVENTS), bank);
	publish_tpu_event_window(bank, ARRAY_SIZE(RF_INITIALIZATION_EVENTS), event_groups);
}

static const struct gsm_scan_range *find_scan_range(uint16_t channel_index) {
	for (size_t i = 0; i < ARRAY_SIZE(GSM_SCAN_RANGES); i++) {
		const struct gsm_scan_range *range = &GSM_SCAN_RANGES[i];

		if (channel_index >= range->table_first && channel_index < range->table_first + range->channel_count)
			return range;
	}

	return 0;
}

static uint16_t channel_index_to_arfcn(uint16_t channel_index) {
	const struct gsm_scan_range *range = find_scan_range(channel_index);

	return range->arfcn_first + channel_index - range->table_first;
}

static uint32_t channel_index_to_frequency_hz(uint16_t channel_index) {
	const struct gsm_scan_range *range = find_scan_range(channel_index);

	return range->frequency_first + (channel_index - range->table_first) * GSM_CHANNEL_RASTER_HZ;
}

static bool publish_rf_transition_frame(uint32_t bank) {
	if (tpu_rf_transition_frame < TPU_RF_QUIET_FRAME_COUNT) {
		uint32_t event_groups = TPU_RF_QUIET_EVENT_GROUPS;
		if (tpu_rf_transition_frame == 0)
			event_groups = TPU_RF_FIRST_QUIET_EVENT_GROUPS;

		write_tpu_schedule(SIGNAL_SEARCH_EVENTS, ARRAY_SIZE(SIGNAL_SEARCH_EVENTS), bank);
		publish_tpu_event_window(bank, ARRAY_SIZE(SIGNAL_SEARCH_EVENTS), event_groups);
		tpu_rf_transition_frame++;
		return true;
	}

	if (tpu_rf_transition_frame == TPU_RF_QUIET_FRAME_COUNT) {
		publish_rf_initialization_frame(bank, TPU_RF_INITIALIZATION_PREFIX_EVENT_GROUPS);
		tpu_rf_transition_frame++;
		return true;
	}

	return false;
}

static void tpu_loop_start(void) {
	write_initial_tpu_schedules();
	gsm_trx_reset_telegram_table();
	gsm_trx_write_telegram_table(0);
	gsm_trx_write_telegram_table(1);

	TPU_GSMCLK1 = 1 << TPU_GSMCLK1_K_SHIFT;
	TPU_GSMCLK2 = 4 << TPU_GSMCLK2_L_SHIFT;
	TPU_GSMCLK3 = TPU_GSMCLK3_INIT;
	TPU_OVERFLOW = TPU_FRAME_TICKS - 1;
	TPU_OFFSET = 0;
	TPU_INT(1) = TPU_FRAME_INTERRUPT_TICK;
	TPU_SRC(1) = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_TPU_INT1_IRQ) = 1;

	tpu_loop_phase = TPU_LOOP_RF_POWER_UP;
	tpu_schedule_bank = 0;
	tpu_rf_transition_frame = 0;
	rf_initialization_complete = false;
	publish_tpu_event_window(0, ARRAY_SIZE(RF_POWER_UP_EVENTS), TPU_RF_POWER_UP_EVENT_GROUPS);

	cpu_enable_irq(true);
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	tpu_loop_running = true;
}

static void tpu_loop_stop(void) {
	if (!tpu_loop_running)
		return;

	TPU_PARAM = 0;
	cpu_enable_irq(false);
	VIC_CON(VIC_TPU_INT1_IRQ) = 0;
	TPU_SRC(1) = MOD_SRC_CLRR;
	tpu_loop_running = false;
}

static bool transceiver_init(void) {
	gsm_trx_init();
	gsm_board_configure_transceiver_clocks();
	tpu_loop_start();

	stopwatch_t start = stopwatch_get();
	while (!rf_initialization_complete && stopwatch_elapsed_ms(start) < TEST_TIMEOUT_MS)
		test_watchdog_serve();
	if (!rf_initialization_complete)
		return false;
	if (!gsm_trx_is_idle())
		return false;

	gsm_board_set_frequency_correction_hz(gsm_board_get_frequency_correction_hz());
	return true;
}

static void configure_monitoring_frame(uint32_t bank, const uint16_t *channels) {
	for (size_t i = 0; i < GSM_TRX_MONITOR_COUNT; i++) {
		if (channels[i] != GSM_CHANNEL_NONE) {
			uint16_t channel_index = channels[i];
			struct channel_scan_result *result = &scan_results[channel_index];

			scan_request_gain_states[i] = result->gain_state;
		} else {
			scan_request_gain_states[i] = gsm_board_get_initial_gain_state();
		}
	}

	gsm_trx_configure_monitoring(bank, channels, scan_request_gain_states);
}

static void reset_fcch_result(void) {
	for (size_t i = 0; i < FC_RESULT_WORDS; i++)
		dsp_hw_shared_memory[FC_STATUS_OFFSET + i] = FC_SENTINEL_BASE + i;
}

static void configure_fcch_receiver(uint32_t bank) {
	struct channel_scan_result *result = &scan_results[fcch_channel_index];

	gsm_trx_configure_fcch(bank, fcch_channel_index, result->gain_state);
}

static void collect_fcch_result(void) {
	struct fcch_result *result = &fcch_results[fcch_candidate_index][fcch_window];

	result->communication_flags = DSP_COM_STATUS & FC_COMMUNICATION_FLAGS;
	result->status = dsp_hw_shared_memory[FC_STATUS_OFFSET];
	result->start = dsp_hw_shared_memory[FC_START_OFFSET];
	result->quality = dsp_hw_shared_memory[FC_QUALITY_OFFSET];
	result->rms = (int16_t) dsp_hw_shared_memory[FC_RMS_OFFSET];
	result->frequency = (int16_t) dsp_hw_shared_memory[FC_FREQUENCY_OFFSET];
	if (result->communication_flags == 0 && result->status == 2) {
		fcch_candidate_detections++;
		fcch_candidate_frequency_sum += result->frequency;
	}

	fcch_window++;
	fcch_candidate_attempts[fcch_candidate_index] = fcch_window;
	if (fcch_window != FCCH_ATTEMPT_COUNT)
		return;

	if (fcch_candidate_detections != 0) {
		int32_t frequency_error_hz = fcch_candidate_frequency_sum / (int32_t) fcch_candidate_detections;
		gsm_board_set_frequency_correction_hz(gsm_board_get_frequency_correction_hz() + frequency_error_hz);
		fcch_scan_state = FCCH_SCAN_COMPLETE;
		return;
	}

	fcch_candidate_index++;
	if (fcch_candidate_index == fcch_candidate_count) {
		fcch_scan_state = FCCH_SCAN_COMPLETE;
		return;
	}

	fcch_channel_index = fcch_candidates[fcch_candidate_index].channel_index;
	fcch_window = 0;
	fcch_candidate_detections = 0;
	fcch_candidate_frequency_sum = 0;
	fcch_scan_state = FCCH_SCAN_BB_OFF_REQUESTED;
}

static void reset_monitor_results(void) {
	for (size_t i = 0; i < MON_VALUE_COUNT; i++)
		dsp_hw_shared_memory[MON_VALUES_OFFSET + i] = 0xFFFF;
}

static void collect_monitor_results(const uint16_t *channels) {
	for (size_t i = 0; i < GSM_TRX_MONITOR_COUNT; i++) {
		uint16_t raw = dsp_hw_shared_memory[MON_VALUES_OFFSET + MON_RESULT_FIRST + i];
		if (raw == 0xFFFF)
			continue;

		if (channels[i] != GSM_CHANNEL_NONE) {
			uint16_t channel_index = channels[i];
			const struct gsm_scan_range *range = find_scan_range(channel_index);
			struct channel_scan_result *result = &scan_results[channel_index];
			uint16_t gain_state = scan_request_gain_states[i];
			uint16_t next_gain_state = gsm_board_calculate_next_gain_state(range->band, channel_index, gain_state, raw);
			int32_t level_dbm_x16 = gsm_board_calculate_rx_level_dbm_x16(range->band, channel_index, gain_state, raw);

			result->level_dbm_x16 = level_dbm_x16;
			result->measurements++;
			result->gain_state = next_gain_state;
		}
		dsp_hw_shared_memory[MON_VALUES_OFFSET + MON_RESULT_FIRST + i] = raw | 0x8000;
	}
}

static void build_channel_scan_request(void) {
	for (size_t i = 0; i < ARRAY_SIZE(scan_request_channels); i++) {
		int position = scan_window_start + (int) i;

		scan_request_channels[i] = position >= 0 && position < GSM_SCAN_CHANNEL_COUNT ?
			scan_channels[position] : GSM_CHANNEL_NONE;
	}
}

static void channel_scan_start_pass(void) {
	uint32_t phase = scan_pass % SCAN_WINDOW_STRIDE;

	/* Rotate four-channel windows over all eight alignments, including wrapped partial windows. */
	scan_window_start = (int) phase;
	if (scan_window_start > GSM_TRX_MONITOR_COUNT)
		scan_window_start -= SCAN_WINDOW_STRIDE;
}

static bool channel_scan_advance(void) {
	if (receiver_scan_state == RECEIVER_SCAN_ACTIVE) {
		if (!scan_request_repeated) {
			scan_request_repeated = true;
			return true;
		}
		collect_monitor_results(scan_request_channels);
		scan_request_repeated = false;
	} else if (receiver_scan_state == RECEIVER_SCAN_REQUESTED) {
		receiver_scan_state = RECEIVER_SCAN_ACTIVE;
		scan_pass = 0;
		scan_request_repeated = false;
		channel_scan_start_pass();
	} else {
		return false;
	}

	if (scan_window_start >= GSM_SCAN_CHANNEL_COUNT) {
		scan_pass++;
		if (scan_pass == SCAN_PASS_COUNT) {
			receiver_scan_state = RECEIVER_SCAN_COMPLETE;
			return false;
		}
		channel_scan_start_pass();
	}

	build_channel_scan_request();
	scan_window_start += SCAN_WINDOW_STRIDE;
	return true;
}

static void publish_fcch_frame(uint32_t bank, uint32_t event_groups) {
	configure_fcch_receiver(bank);
	write_tpu_schedule(FCCH_EVENTS, ARRAY_SIZE(FCCH_EVENTS), bank);
	publish_tpu_event_window(bank, ARRAY_SIZE(FCCH_EVENTS), event_groups);
}

static void fcch_scan_start(uint32_t bank) {
	tpu_loop_phase = TPU_LOOP_FCCH;
	fcch_scan_state = FCCH_SCAN_BB_OFF_REQUESTED;
	fcch_candidate_index = 0;
	fcch_window = 0;
	fcch_candidate_detections = 0;
	fcch_candidate_frequency_sum = 0;
	fcch_channel_index = fcch_candidates[0].channel_index;
	fcch_tpu_phase = FCCH_TPU_RF_QUIET_FIRST;

	gsm_trx_reset_telegram_table();
	gsm_trx_write_telegram_table(0);
	gsm_trx_write_telegram_table(1);
	publish_rf_initialization_frame(bank, TPU_RF_QUIET_EVENT_GROUPS);
}

static void publish_next_tpu_frame(void) {
	uint32_t search_event_groups = TPU_SEARCH_EVENT_GROUPS;
	bool repeat_transition_bank = tpu_loop_phase == TPU_LOOP_READY && tpu_rf_transition_frame == 1;

	if (!repeat_transition_bank)
		tpu_schedule_bank ^= 1;
	if (tpu_loop_phase == TPU_LOOP_RF_POWER_UP) {
		tpu_loop_phase = TPU_LOOP_RF_INITIALIZATION;
		publish_rf_initialization_frame(tpu_schedule_bank, TPU_INITIALIZATION_EVENT_GROUPS);
		return;
	}

	if (tpu_loop_phase == TPU_LOOP_READY) {
		if (publish_rf_transition_frame(tpu_schedule_bank))
			return;

		if (receiver_scan_state == RECEIVER_SCAN_REQUESTED) {
			tpu_loop_phase = TPU_LOOP_SEARCH;
			search_event_groups = TPU_FIRST_SEARCH_EVENT_GROUPS;
		} else {
			write_tpu_schedule(TPU_IDLE_EVENTS, ARRAY_SIZE(TPU_IDLE_EVENTS), tpu_schedule_bank);
			publish_tpu_event_window(tpu_schedule_bank, ARRAY_SIZE(TPU_IDLE_EVENTS), TPU_SEARCH_EVENT_GROUPS);
			return;
		}
	}

	if (tpu_loop_phase == TPU_LOOP_SEARCH) {
		if (channel_scan_advance()) {
			configure_monitoring_frame(tpu_schedule_bank, scan_request_channels);
			write_tpu_schedule(SIGNAL_SEARCH_EVENTS, ARRAY_SIZE(SIGNAL_SEARCH_EVENTS), tpu_schedule_bank);
			dsp_hw_shared_memory[MON_INDEX_OFFSET] = MON_RESULT_FIRST;
			publish_tpu_event_window(tpu_schedule_bank, ARRAY_SIZE(SIGNAL_SEARCH_EVENTS), search_event_groups);
			return;
		}

		if (receiver_scan_state == RECEIVER_SCAN_COMPLETE && fcch_scan_state == FCCH_SCAN_REQUESTED) {
			fcch_scan_start(tpu_schedule_bank);
			return;
		}

		write_tpu_schedule(TPU_IDLE_EVENTS, ARRAY_SIZE(TPU_IDLE_EVENTS), tpu_schedule_bank);
		publish_tpu_event_window(tpu_schedule_bank, ARRAY_SIZE(TPU_IDLE_EVENTS), TPU_SEARCH_EVENT_GROUPS);
		return;
	}

	if (tpu_loop_phase == TPU_LOOP_FCCH) {
		if (fcch_tpu_phase == FCCH_TPU_RF_QUIET_FIRST) {
			if (fcch_scan_state == FCCH_SCAN_BB_OFF_REQUESTED) {
				publish_rf_initialization_frame(tpu_schedule_bank, TPU_RF_QUIET_EVENT_GROUPS);
				return;
			}

			fcch_tpu_phase = FCCH_TPU_RF_QUIET_SECOND;
			publish_rf_initialization_frame(tpu_schedule_bank, TPU_RF_QUIET_EVENT_GROUPS);
			return;
		}

		if (fcch_tpu_phase == FCCH_TPU_RF_QUIET_SECOND) {
			fcch_tpu_phase = FCCH_TPU_RF_INITIALIZATION;
			publish_rf_initialization_frame(tpu_schedule_bank, TPU_INITIALIZATION_EVENT_GROUPS);
			return;
		}

		if (fcch_tpu_phase == FCCH_TPU_RF_INITIALIZATION) {
			fcch_tpu_phase = FCCH_TPU_PREPARE_FIRST;
			publish_fcch_frame(tpu_schedule_bank, TPU_FCCH_PREPARE_EVENT_GROUPS);
			return;
		}

		if (fcch_tpu_phase == FCCH_TPU_PREPARE_FIRST) {
			fcch_tpu_phase = FCCH_TPU_PREPARE_SECOND;
			fcch_scan_state = FCCH_SCAN_COMMAND_REQUESTED;
			publish_fcch_frame(tpu_schedule_bank, TPU_FCCH_PREPARE_EVENT_GROUPS);
			return;
		}

		if (fcch_tpu_phase == FCCH_TPU_PREPARE_SECOND) {
			if (fcch_scan_state == FCCH_SCAN_COMMAND_REQUESTED) {
				publish_fcch_frame(tpu_schedule_bank, TPU_FCCH_HOLD_EVENT_GROUPS);
				return;
			}

			fcch_tpu_phase = FCCH_TPU_START;
			publish_fcch_frame(tpu_schedule_bank, TPU_FCCH_START_EVENT_GROUPS);
			return;
		}

		if (fcch_tpu_phase == FCCH_TPU_START) {
			fcch_tpu_phase = FCCH_TPU_HOLD_FIRST;
			publish_fcch_frame(tpu_schedule_bank, TPU_FCCH_HOLD_EVENT_GROUPS);
			return;
		}

		if (fcch_tpu_phase == FCCH_TPU_HOLD_FIRST) {
			fcch_tpu_phase = FCCH_TPU_HOLD_SECOND;
			publish_fcch_frame(tpu_schedule_bank, TPU_FCCH_HOLD_EVENT_GROUPS);
			return;
		}

		if (fcch_tpu_phase == FCCH_TPU_HOLD_SECOND) {
			fcch_tpu_phase = FCCH_TPU_FINISH;
			publish_fcch_frame(tpu_schedule_bank, TPU_FCCH_PREPARE_EVENT_GROUPS);
			return;
		}

		if ((DSP_COM_STATUS & FC_COMMUNICATION_FLAGS) != 0) {
			publish_fcch_frame(tpu_schedule_bank, TPU_FCCH_HOLD_EVENT_GROUPS);
			return;
		}

		collect_fcch_result();
		if (fcch_scan_state == FCCH_SCAN_COMPLETE) {
			tpu_loop_phase = TPU_LOOP_COMPLETE;
		} else if (fcch_scan_state == FCCH_SCAN_BB_OFF_REQUESTED) {
			fcch_tpu_phase = FCCH_TPU_RF_QUIET_FIRST;
			gsm_trx_reset_telegram_table();
			gsm_trx_write_telegram_table(0);
			gsm_trx_write_telegram_table(1);
			publish_rf_initialization_frame(tpu_schedule_bank, TPU_RF_QUIET_EVENT_GROUPS);
			return;
		} else {
			fcch_tpu_phase = FCCH_TPU_PREPARE_SECOND;
			fcch_scan_state = FCCH_SCAN_COMMAND_REQUESTED;
			publish_fcch_frame(tpu_schedule_bank, TPU_FCCH_PREPARE_EVENT_GROUPS);
			return;
		}
	}

	write_tpu_schedule(TPU_IDLE_EVENTS, ARRAY_SIZE(TPU_IDLE_EVENTS), tpu_schedule_bank);
	publish_tpu_event_window(tpu_schedule_bank, ARRAY_SIZE(TPU_IDLE_EVENTS), TPU_SEARCH_EVENT_GROUPS);
}

static bool wait_for_dsp_communication(uint16_t flags) {
	stopwatch_t start = stopwatch_get();
	while ((DSP_COM_STATUS & flags) != 0 && stopwatch_elapsed_ms(start) < TEST_TIMEOUT_MS)
		test_watchdog_serve();

	return (DSP_COM_STATUS & flags) == 0;
}

static void signal_dsp_communication(uint16_t flags) {
	DSP_COM_SET = flags;
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;
}

static void dsp_runtime_command_start(uint16_t command, const uint16_t *parameters, size_t count) {
	dsp_hw_shared_memory[RUNTIME_PIPE_OFFSET] = command;
	for (size_t i = 0; i < count; i++)
		dsp_hw_shared_memory[RUNTIME_PIPE_OFFSET + 1 + i] = parameters[i];

	signal_dsp_communication(BIT(0));
}

static bool dsp_runtime_command_execute(uint16_t command, const uint16_t *parameters, size_t count) {
	dsp_runtime_command_start(command, parameters, count);
	return wait_for_dsp_communication(BIT(0));
}

static bool dsp_execute_baseband_off(void) {
	uint16_t sequence = dsp_hw_shared_memory[BB_OFF_SEQUENCE_OFFSET] & 0xFF00;
	sequence++;
	dsp_hw_shared_memory[BB_OFF_SEQUENCE_OFFSET] = sequence;
	return dsp_runtime_command_execute(DSP_COMMAND_BB_OFF, NULL, 0);
}

static void dsp_fcch_command_start(void) {
	/* Firmware 0801 search-mode payload from dsp_fc_init(0, 0x21, 0x73). */
	static const uint16_t PARAMETERS[] = { 1, 115, 33 };

	reset_fcch_result();
	dsp_runtime_command_start(DSP_COMMAND_FC_INIT, PARAMETERS, ARRAY_SIZE(PARAMETERS));
}

static bool dsp_channel_decoder_profiles_init(void) {
	/* Exact command payloads from firmware 0801 dsp_channel_decoder_init_profiles at 0xA0A748C8. */
	static const struct {
		uint16_t parameters[27];
		size_t count;
	} PROFILES[] = {
		{ { 0, 0x10, 2, 0x14F5, 0, 0x14F5, 0x4C2C, 0x10, 2, 0x14F5, 0, 0x14F5, 0x4C2C }, 13 },
		{ { 2, 0x0C, 3, 0x0B86, 0x0BAE, 0x0BEA, 0x2BF2, 0x0C, 3, 0x0B86, 0x0BAE, 0x0BEA, 0x2BF2 }, 13 },
		{ { 1, 0x10, 2, 0x14F5, 0, 0x14F5, 0x4C2C, 0x10, 2, 0x14F5, 0, 0x14F5, 0x4C2C }, 13 },
		{ { 3, 0, 0x1026, 0x1026, 0x115E, 0x10FE }, 6 },
		{ { 3, 1, 0x12A2, 0x12A2, 0x0951, 0x12A2, 0x12A2, 0x0951, 0x10FE }, 9 },
		{ { 3, 2, 0x0074, 0x0217, 0x0072, 0x0235, 0x006B, 0x0208, 0x0067, 0x0240 }, 10 },
		{ { 3, 3, 0x0062, 0x01DA, 0x005D, 0x0201, 0x0053, 0x0282, 0x004D, 0x01FC }, 10 },
		{ { 3, 4, 0x0025, 0x011D, 0x0024, 0x012F, 0x0020, 0x00E0 }, 8 },
		{ { 3, 5, 0x001C, 0x00F0, 0x001A, 0x0104, 0x0017, 0x010A }, 8 },
		{ { 3, 6, 0x00D4 }, 3 },
		{ { 3, 7, 0, 0, 0 }, 5 },
		{ { 3, 8, 0x1635, 0x1B6C, 0x209E, 0x1644, 0x1B6C, 0x203A, 0x1644, 0x1AC2, 0x1EAF, 0x164E,
			0x19FA, 0x1E05, 0x16DA, 0x19FA, 0x1D1F, 0x16DA, 0x199B, 0x1C61, 0x16F8, 0x1919, 0x1B26,
			0x1734, 0x18F1, 0x1A40, 0x5B68 }, 27 },
		{ { 3, 9, 0x0B18, 0x0C8A, 0x0D8E, 0x0B22, 0x0C8A, 0x0D6B, 0x0B2C, 0x0C80, 0x0D0C, 0x0B36,
			0x0C80, 0x0CC6, 0x0B54, 0x0C80, 0x0C85, 0x0B68, 0x0C80, 0x0C6C, 0x2CEC }, 21 },
	};

	for (size_t i = 0; i < ARRAY_SIZE(PROFILES); i++)
		if (!dsp_runtime_command_execute(DSP_COMMAND_DEC_INIT, PROFILES[i].parameters, PROFILES[i].count))
			return false;

	return true;
}

static bool dsp_receiver_init(void) {
	static const uint16_t RF_ADAPT_PARAMETERS[] = { 0, 0x0599, 1, TEAK_BB_CTRL_BBADAP_EN, 0 };
	static const uint16_t MODU_INIT_PARAMETERS[] = { 0xFFFB, 0xFFE7, 220, 220, 0, 21, 1, 0, 8 };
	static const uint16_t IQ_SWAP_PARAMETERS[] = { 1 };

	if (!dsp_hw_reset())
		return false;
	for (size_t i = 0; i < TEAK_SHARED_RAM_SIZE / sizeof(*dsp_hw_shared_memory); i++)
		dsp_hw_shared_memory[i] = 0;

	DSP_COM_CLEAR = 0xFFFF;
	if (!dsp_hw_load_container(DSP_PMB8876_FIRMWARE_0801_CONTAINER, sizeof(DSP_PMB8876_FIRMWARE_0801_CONTAINER)))
		return false;
	stopwatch_usleep_wd(1000);

	if (!dsp_channel_decoder_profiles_init())
		return false;
	if (!dsp_runtime_command_execute(DSP_COMMAND_MODU_INIT, MODU_INIT_PARAMETERS, ARRAY_SIZE(MODU_INIT_PARAMETERS)))
		return false;
	dsp_hw_shared_memory[RF_ADAPT_EXTENSION_OFFSET] = RF_ADAPT_EXTENSION_VALUE;
	if (!dsp_runtime_command_execute(DSP_COMMAND_RF_ADAPT, RF_ADAPT_PARAMETERS, ARRAY_SIZE(RF_ADAPT_PARAMETERS)))
		return false;
	dsp_hw_shared_memory[DSP_RUNTIME_CONFIG_OFFSET] = DSP_RUNTIME_CONFIG_VALUE;
	if (!dsp_runtime_command_execute(DSP_COMMAND_IDLE, NULL, 0))
		return false;
	if (!dsp_runtime_command_execute(DSP_COMMAND_IQ_SWAP_1, IQ_SWAP_PARAMETERS, ARRAY_SIZE(IQ_SWAP_PARAMETERS)))
		return false;
	if (!dsp_runtime_command_execute(DSP_COMMAND_IQ_SWAP_2, IQ_SWAP_PARAMETERS, ARRAY_SIZE(IQ_SWAP_PARAMETERS)))
		return false;
	if (!dsp_runtime_command_execute(DSP_COMMAND_MODU_INIT, MODU_INIT_PARAMETERS, ARRAY_SIZE(MODU_INIT_PARAMETERS)))
		return false;
	if (!dsp_runtime_command_execute(DSP_COMMAND_IDLE, NULL, 0))
		return false;

	dsp_hw_shared_memory[MON_INDEX_OFFSET] = 0;
	reset_monitor_results();
	return true;
}

static void update_scan_stats(struct scan_stats *stats, const struct channel_scan_result *result, uint32_t frequency) {
	stats->channels++;
	stats->monitor_samples += result->measurements;
	if (result->measurements == 0)
		return;

	if (stats->valid_channels == 0) {
		stats->baseline = result->level_dbm_x16;
		stats->minimum = result->level_dbm_x16;
		stats->maximum = result->level_dbm_x16;
		stats->minimum_frequency = frequency;
		stats->maximum_frequency = frequency;
	} else {
		if (result->level_dbm_x16 != stats->baseline)
			stats->different_channels++;
		if (result->level_dbm_x16 < stats->minimum) {
			stats->minimum = result->level_dbm_x16;
			stats->minimum_frequency = frequency;
		}
		if (result->level_dbm_x16 > stats->maximum) {
			stats->maximum = result->level_dbm_x16;
			stats->maximum_frequency = frequency;
		}
	}
	stats->valid_channels++;
}

static int32_t round_dbm_x16(int32_t dbm_x16) {
	if (dbm_x16 < 0)
		return -((-dbm_x16 + MON_RAW_VALUES_PER_DB / 2) / MON_RAW_VALUES_PER_DB);
	return (dbm_x16 + MON_RAW_VALUES_PER_DB / 2) / MON_RAW_VALUES_PER_DB;
}

static void print_scan_stats(const char *scope, const struct scan_stats *stats) {
	int32_t spread_db = (stats->maximum - stats->minimum + MON_RAW_VALUES_PER_DB / 2) / MON_RAW_VALUES_PER_DB;

	printf("# GSM_SCAN_STATS,scope=%s,channels=%u,valid=%u,samples=%u,baseline_dbm=%d,different=%u,"
		"min_dbm=%d,min_frequency=%u,max_dbm=%d,max_frequency=%u,spread_db=%d\n", scope, stats->channels,
		stats->valid_channels, stats->monitor_samples, round_dbm_x16(stats->baseline), stats->different_channels,
		round_dbm_x16(stats->minimum), stats->minimum_frequency,
		round_dbm_x16(stats->maximum), stats->maximum_frequency, spread_db);
}

static void add_fcch_candidate(uint16_t channel_index, int16_t level_dbm_x16) {
	size_t position = 0;
	while (position < fcch_candidate_count && fcch_candidates[position].level_dbm_x16 >= level_dbm_x16)
		position++;
	if (position == FCCH_CANDIDATE_COUNT)
		return;

	size_t last = fcch_candidate_count < FCCH_CANDIDATE_COUNT ? fcch_candidate_count : FCCH_CANDIDATE_COUNT - 1;
	for (size_t i = last; i > position; i--)
		fcch_candidates[i] = fcch_candidates[i - 1];
	if (fcch_candidate_count < FCCH_CANDIDATE_COUNT)
		fcch_candidate_count++;

	fcch_candidates[position].channel_index = channel_index;
	fcch_candidates[position].arfcn = channel_index_to_arfcn(channel_index);
	fcch_candidates[position].level_dbm_x16 = level_dbm_x16;
}

static void print_fcch_candidates(void) {
	for (size_t i = 0; i < fcch_candidate_count; i++) {
		const struct fcch_candidate *candidate = &fcch_candidates[i];
		const struct gsm_scan_range *range = find_scan_range(candidate->channel_index);

		printf("# FCCH_CANDIDATE,rank=%u,band=%s,arfcn=%u,frequency=%u,level_dbm=%d\n",
			(uint32_t) i + 1, range->name, (uint32_t) candidate->arfcn,
			channel_index_to_frequency_hz(candidate->channel_index), round_dbm_x16(candidate->level_dbm_x16));
	}
}

static void discard_background_candidates(int16_t background_level) {
	while (fcch_candidate_count > 1 && fcch_candidates[fcch_candidate_count - 1].level_dbm_x16 <= background_level)
		fcch_candidate_count--;
}

static void build_channel_scan_order(void) {
	size_t position = 0;

	for (size_t range_index = 0; range_index < ARRAY_SIZE(GSM_SCAN_RANGES); range_index++) {
		const struct gsm_scan_range *range = &GSM_SCAN_RANGES[range_index];

		for (uint16_t offset = 0; offset < range->channel_count; offset++)
			scan_channels[position++] = range->table_first + offset;
	}
}

static bool channel_scan_execute(void) {
	struct scan_stats band_stats[GSM_BAND_COUNT] = { 0 };
	struct scan_stats all_stats = { 0 };

	fcch_candidate_count = 0;
	for (size_t i = 0; i < ARRAY_SIZE(scan_channels); i++) {
		struct channel_scan_result *result = &scan_results[scan_channels[i]];

		result->level_dbm_x16 = 0;
		result->measurements = 0;
		result->gain_state = gsm_board_get_initial_gain_state();
	}
	for (size_t i = 0; i < ARRAY_SIZE(fcch_candidate_attempts); i++)
		fcch_candidate_attempts[i] = 0;

	receiver_scan_state = RECEIVER_SCAN_REQUESTED;
	stopwatch_t start = stopwatch_get();
	while (receiver_scan_state != RECEIVER_SCAN_COMPLETE && stopwatch_elapsed_ms(start) < SCAN_TIMEOUT_MS)
		test_watchdog_serve();

	for (size_t i = 0; i < ARRAY_SIZE(scan_channels); i++) {
		uint16_t channel_index = scan_channels[i];
		const struct gsm_scan_range *range = find_scan_range(channel_index);
		const struct channel_scan_result *result = &scan_results[channel_index];
		uint32_t frequency = channel_index_to_frequency_hz(channel_index);

		update_scan_stats(&band_stats[range->band], result, frequency);
		update_scan_stats(&all_stats, result, frequency);
		if (result->measurements == 0)
			return false;
		add_fcch_candidate(channel_index, result->level_dbm_x16);
	}

	for (size_t band = 0; band < GSM_BAND_COUNT; band++) {
		for (size_t range_index = 0; range_index < ARRAY_SIZE(GSM_SCAN_RANGES); range_index++) {
			if (GSM_SCAN_RANGES[range_index].band == band) {
				print_scan_stats(GSM_SCAN_RANGES[range_index].name, &band_stats[band]);
				break;
			}
		}
	}
	print_scan_stats("all", &all_stats);
	discard_background_candidates(all_stats.minimum);
	print_fcch_candidates();

	fcch_scan_state = FCCH_SCAN_REQUESTED;
	return true;
}

static bool fcch_scan_execute(void) {
	stopwatch_t start = stopwatch_get();

	while (fcch_scan_state != FCCH_SCAN_COMPLETE && stopwatch_elapsed_ms(start) < FCCH_TIMEOUT_MS) {
		if (fcch_scan_state == FCCH_SCAN_BB_OFF_REQUESTED) {
			if (!dsp_execute_baseband_off()) {
				printf("# FCCH_COMMAND_TIMEOUT,command=BB_OFF,dsp_status=%04X\n", (uint32_t) DSP_COM_STATUS);
				return false;
			}
			fcch_scan_state = FCCH_SCAN_ACTIVE;
		}
		if (fcch_scan_state == FCCH_SCAN_COMMAND_REQUESTED) {
			dsp_fcch_command_start();
			fcch_scan_state = FCCH_SCAN_ACTIVE;
		}
		test_watchdog_serve();
	}

	uint32_t detected = 0;
	uint32_t windows = 0;
	uint32_t candidates_tested = 0;
	for (size_t candidate = 0; candidate < fcch_candidate_count; candidate++) {
		if (fcch_candidate_attempts[candidate] != 0)
			candidates_tested++;
		for (size_t attempt = 0; attempt < fcch_candidate_attempts[candidate]; attempt++) {
			const struct fcch_candidate *fcch_candidate = &fcch_candidates[candidate];
			const struct gsm_scan_range *range = find_scan_range(fcch_candidate->channel_index);
			const struct fcch_result *result = &fcch_results[candidate][attempt];

			if (result->status == 2)
				detected++;
			printf("# FCCH,candidate=%u,window=%u,band=%s,arfcn=%u,status=%u,start=%u,quality=%u,rms=%d,"
				"frequency=%d,com=%04X\n", (uint32_t) candidate + 1, (uint32_t) attempt + 1, range->name,
				(uint32_t) fcch_candidate->arfcn, (uint32_t) result->status,
				(uint32_t) result->start, (uint32_t) result->quality, (int32_t) result->rms,
				(int32_t) result->frequency, (uint32_t) result->communication_flags);
			windows++;
		}
	}
	printf("# FCCH_STATS,candidates=%u,windows=%u,detected=%u,afc_frequency=%d,afc_value=%04X\n",
		candidates_tested, windows, detected, gsm_board_get_frequency_correction_hz(),
		(uint32_t) gsm_board_get_afc_value());
	if (fcch_scan_state != FCCH_SCAN_COMPLETE)
		printf("# FCCH_TIMEOUT,state=%u,phase=%u,dsp_status=%04X\n", (uint32_t) fcch_scan_state,
			(uint32_t) fcch_tpu_phase, (uint32_t) DSP_COM_STATUS);

	return fcch_scan_state == FCCH_SCAN_COMPLETE;
}

static bool receiver_init(void) {
	if (!gsm_board_init())
		return false;

	dsp_clock_enabled = true;
	clear_tpu_ram();
	return dsp_receiver_init();
}

int main(void) {
	build_channel_scan_order();

	test_start("GSM channel scan");
	test_category("Receiver setup");
	bool receiver_ready = test_check("Baseband receive path initializes once", receiver_init());

	bool transceiver_ready = false;
	if (receiver_ready) {
		gsm_board_enable_transceiver_power();
		transceiver_ready = test_check("RF transceiver initializes through TPU control RAM", transceiver_init());
	}

	test_category("GSM channel scan");
	wdt_set_max_execution_time(SCAN_TIMEOUT_MS);
	bool scan_completed = transceiver_ready && channel_scan_execute();
	test_check("All supported GSM bands scan", scan_completed);

	test_category("FCCH and AFC");
	wdt_set_max_execution_time(FCCH_TIMEOUT_MS);
	bool fcch_completed = scan_completed && fcch_scan_execute();
	test_check("FCCH scan completes", fcch_completed);

	tpu_loop_stop();
	if (dsp_clock_enabled) {
		DSP_COM_CLEAR = 0xFFFF;
		(void) dsp_hw_reset();
	}
	return test_finish();
}

__IRQ void irq_handler(void) {
	uint32_t irq_number = VIC_IRQ_CURRENT;

	if (irq_number == VIC_TPU_INT1_IRQ) {
		TPU_SRC(1) |= MOD_SRC_CLRR;
		if (tpu_loop_phase == TPU_LOOP_RF_INITIALIZATION) {
			rf_initialization_complete = true;
			tpu_loop_phase = TPU_LOOP_READY;
		}
		publish_next_tpu_frame();
	}

	VIC_IRQ_ACK = 1;
}
