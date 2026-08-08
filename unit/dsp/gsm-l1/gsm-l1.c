#include <pmb887x.h>
#include <gen/dsp.h>
#include <stopwatch.h>

#include "../dsp-hw.h"
#include "board/gsm.h"
#include "gsm-l1.h"
#include "gsm-trx.h"
#include "gsm.h"
#include "el71-v45-dsp-container.inc"

#define TPU_TIMER_RAM_BASE 512
#define TPU_TIMER_BANK_0_OFFSET 0
#define TPU_TIMER_BANK_1_OFFSET 0xFF
#define TPU_EVENT_WORD_COUNT 3

#define TPU_EVENT_GROUP_SHIFT 9
#define TPU_EVENT_TRIGGER_HIGH_SHIFT 16
#define TPU_EVENT_TRIGGER_HIGH_MASK 0x1FF
#define TPU_EVENT_TRIGGER_25_SHIFT 25
#define TPU_EVENT_TIME_TRIGGER_SHIFT 15
#define TPU_EVENT_TRIGGER_LOW_MASK 0xFFFF

#define TPU_TRIGGER_RF_MASK 0x3F
#define TPU_TRIGGER_DECODER_SHIFT 6
#define TPU_TRIGGER_ADCTRIG_BIT 11
#define TPU_TRIGGER_T_OUT_SHIFT 12

/* SET/CLEAR replace TRIG[11:0], update TRIG[24:12], and copy TRIG[25]. */
enum tpu_event_operation {
	TPU_EVENT_REPLACE = 0x0000,
	TPU_EVENT_SET = 0x4000,
	TPU_EVENT_CLEAR = 0x8000,
	TPU_EVENT_TIMING_ADVANCE = 0xC000,
};

enum tpu_timer_decoder {
	TPU_TIMER_DECODER_NONE = 0,
	TPU_TIMER_DECODER_EQON_SET = 1,
	TPU_TIMER_DECODER_MONON_SET = 2,
	TPU_TIMER_DECODER_SCON_SET = 3,
	TPU_TIMER_DECODER_FCON_SET = 4,
	/* Clears EQON, MONON, SCON, and FCON. */
	TPU_TIMER_DECODER_RECEIVE_CLEAR = 5,
	TPU_TIMER_DECODER_RXON_SET = 6,
	TPU_TIMER_DECODER_RXON_CLEAR = 7,
	TPU_TIMER_DECODER_INT_GP0 = 10,
	TPU_TIMER_DECODER_FRAME_INT = 15,
};

#define TPU_TRIGGER_RF(index) ((index) & TPU_TRIGGER_RF_MASK)
#define TPU_TRIGGER_DECODER(decoder) ((decoder) << TPU_TRIGGER_DECODER_SHIFT)
#define TPU_TRIGGER_ADCTRIG BIT(TPU_TRIGGER_ADCTRIG_BIT)
#define TPU_TRIGGER_T_OUT(index) BIT(TPU_TRIGGER_T_OUT_SHIFT + (index))

#define TPU_RF_TELEGRAM_TRIGGER_FIRST 8
#define TPU_FRAME_TICKS 10000
#define GSM_BITS_PER_FRAME 1250
#define TPU_TICKS_PER_GSM_BIT 8
#define FCCH_PHASE_ADVANCE_TICKS 72
#define TPU_SCH_RESULT_BOUNDARY_TICK 0x0E8B

enum dsp_runtime_command {
	DSP_RUNTIME_COMMAND_FC_INIT = 1,
	DSP_RUNTIME_COMMAND_MODU_INIT = 2,
	DSP_RUNTIME_COMMAND_IQ_SWAP_1 = 3,
	DSP_RUNTIME_COMMAND_IQ_SWAP_2 = 4,
	DSP_RUNTIME_COMMAND_DEC_INIT = 5,
	DSP_RUNTIME_COMMAND_BB_OFF = 13,
	DSP_RUNTIME_COMMAND_IDLE = 14,
	DSP_RUNTIME_COMMAND_RF_ADAPT = 67,
};

#define DSP_MON_RAW_VALUES_PER_DB 16
#define GSM_SCAN_CHANNEL_COUNT (GSM_CHANNEL_COUNT - 1)

#define DSP_RUNTIME_PIPE_OFFSET 0x0005
#define DSP_RF_ADAPT_EXTENSION_OFFSET 0x0018
#define DSP_RF_ADAPT_EXTENSION_VALUE 0xFB8C
#define DSP_BB_OFF_SEQUENCE_OFFSET 0x0500
#define DSP_RUNTIME_CONFIG_OFFSET 0x057A
#define DSP_RUNTIME_CONFIG_VALUE 0xC000

#define DSP_MON_INDEX_OFFSET 191
#define DSP_MON_VALUES_OFFSET 192
#define DSP_MON_VALUE_COUNT 8
#define DSP_MON_RESULT_FIRST 4

#define DSP_FCCH_STATUS_OFFSET 89
#define DSP_FCCH_START_OFFSET 90
#define DSP_FCCH_QUALITY_OFFSET 91
#define DSP_FCCH_RMS_OFFSET 92
#define DSP_FCCH_FREQUENCY_OFFSET 93
#define DSP_FCCH_RESULT_WORD_COUNT 5
#define DSP_FCCH_SENTINEL_BASE 0xA500
#define DSP_FCCH_COMMUNICATION_FLAGS (BIT(3) | BIT(4))
#define DSP_FCCH_PREPARE_COMMUNICATION_FLAG BIT(7)

#define DSP_SCH_EQUALIZER_POSITION_OFFSET 103
#define DSP_SCH_METRIC_OFFSET 112
#define DSP_SCH_STATUS_OFFSET 113
#define DSP_SCH_DATA_OFFSET 114
#define DSP_SCH_RESULT_WORD_COUNT 4
#define DSP_SCH_SENTINEL_BASE 0x5A00
#define DSP_SCH_COMMUNICATION_FLAG BIT(3)

/* EL71 v45 enables monitored candidates at -111 dBm and observes at most 12 FCCH search windows. */
#define FCCH_MIN_RX_LEVEL_DBM_X16 (-111 * DSP_MON_RAW_VALUES_PER_DB)
#define FCCH_ADJACENT_SUPPRESSION_DBM_X16 (8 * DSP_MON_RAW_VALUES_PER_DB)
#define FCCH_SEARCH_WINDOW_COUNT 12
#define FCCH_REFINEMENT_ATTEMPT_COUNT 12
#define SCH_ATTEMPT_COUNT 12
#define SCAN_PASS_COUNT 16
#define SCAN_WINDOW_STRIDE 8
#define TPU_RF_QUIET_FRAME_COUNT 4

/* RF power-up, initialization, and RSSI scan event groups. */
#define TPU_RF_FIRST_QUIET_EVENT_GROUPS 0x8000C000
#define TPU_RF_QUIET_EVENT_GROUPS 0x80004000
#define TPU_RF_INITIALIZATION_PREFIX_EVENT_GROUPS 0x8000400A
#define TPU_FIRST_SEARCH_EVENT_GROUPS 0x80004003
#define TPU_SEARCH_EVENT_GROUPS 0x8000C003
#define TPU_RF_POWER_UP_EVENT_GROUPS 0x8000C000

/* FCCH event groups. */
#define TPU_FCCH_PREPARE_EVENT_GROUPS 0x8000C041
#define TPU_FCCH_START_EVENT_GROUPS 0x8000C003
#define TPU_FCCH_HOLD_EVENT_GROUPS 0x8000C001

/* SCH event groups. */
#define TPU_INITIALIZATION_EVENT_GROUPS 0x8000C00A
#define TPU_SCH_EVENT_GROUPS 0x8000C003

struct tpu_event {
	enum tpu_event_operation operation;
	uint8_t group;
	uint16_t tick;
	uint32_t triggers;
};

enum tpu_loop_phase {
	TPU_LOOP_RF_POWER_UP,
	TPU_LOOP_RF_INITIALIZATION,
	TPU_LOOP_READY,
	TPU_LOOP_SEARCH,
	TPU_LOOP_FCCH,
	TPU_LOOP_SCH,
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
	FCCH_SCAN_REFINEMENT_BB_OFF_REQUESTED,
	FCCH_SCAN_REFINEMENT_COMMAND_REQUESTED,
	FCCH_SCAN_REFINEMENT_ACTIVE,
	FCCH_SCAN_REFINEMENT_COMPLETE,
	FCCH_SCAN_REFINEMENT_FAILED,
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
	FCCH_TPU_REFINEMENT_PREPARE,
	FCCH_TPU_REFINEMENT_START,
	FCCH_TPU_REFINEMENT_WAIT_RESULT,
};

enum sch_scan_state {
	SCH_SCAN_IDLE,
	SCH_SCAN_REQUESTED,
	SCH_SCAN_ACTIVE,
	SCH_SCAN_COMPLETE,
};

enum sch_tpu_phase {
	SCH_TPU_RECEIVE,
	SCH_TPU_RESULT_BOUNDARY,
};

struct gsm_scan_range {
	enum gsm_band band;
	uint16_t table_first;
	uint16_t arfcn_first;
	uint16_t channel_count;
};

struct fcch_result {
	uint16_t status;
	uint16_t start;
	uint16_t quality;
	int16_t rms;
	int16_t frequency;
	uint16_t communication_flags;
};

struct sch_result {
	uint16_t metric;
	uint16_t status;
	uint16_t data[2];
	int16_t equalizer_position;
	uint16_t communication_flags;
};

struct channel_scan_result {
	int16_t level_dbm_x16;
	uint32_t measurements;
	uint16_t gain_state;
	bool fcch_excluded;
};

static const struct gsm_scan_range GSM_SCAN_RANGES[] = {
	{ GSM_BAND_900, 0, 0, 125 },
	{ GSM_BAND_900, 126, 975, 49 },
	{ GSM_BAND_DCS1800, 175, 512, 374 },
	{ GSM_BAND_PCS1900, 549, 512, 299 },
	{ GSM_BAND_850, 848, 128, 124 },
};

/* TPU RF initialization window captured from EL71 v45 in QEMU. */
static const struct tpu_event RF_INITIALIZATION_EVENTS[] = {
	{ TPU_EVENT_SET, 0, 0x0001, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_FRAME_INT) },
	{ TPU_EVENT_CLEAR, 3, 0x0004, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RECEIVE_CLEAR) },
	{ TPU_EVENT_CLEAR, 3, 0x0190, TPU_TRIGGER_T_OUT(2) | TPU_TRIGGER_T_OUT(3) },
	{ TPU_EVENT_SET, 3, 0x07D0, TPU_TRIGGER_RF(8) },
	{ TPU_EVENT_SET, 3, 0x0834, TPU_TRIGGER_RF(9) },
	{ TPU_EVENT_SET, 3, 0x0898, TPU_TRIGGER_RF(10) },
	{ TPU_EVENT_SET, 3, 0x08FC, TPU_TRIGGER_RF(11) },
	{ TPU_EVENT_SET, 3, 0x0960, TPU_TRIGGER_RF(12) },
	{ TPU_EVENT_SET, 3, 0x09C4, TPU_TRIGGER_RF(13) },
	{ TPU_EVENT_SET, 3, 0x0A28, TPU_TRIGGER_RF(14) },
	{ TPU_EVENT_SET, 3, 0x0A8C, TPU_TRIGGER_RF(15) },
	{ TPU_EVENT_SET, 3, 0x0AF0, TPU_TRIGGER_RF(16) },
	{ TPU_EVENT_SET, 3, 0x0B54, TPU_TRIGGER_RF(17) },
	{ TPU_EVENT_SET, 31, 0x7FF8, TPU_TRIGGER_T_OUT(0) },
};

/* Four-monitor search window captured from EL71 v45 in QEMU. */
static const struct tpu_event SIGNAL_SEARCH_EVENTS[] = {
	{ TPU_EVENT_SET, 0, 0x0001, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_FRAME_INT) },
	{ TPU_EVENT_SET, 1, 0x0A10, TPU_TRIGGER_RF(8) },
	{ TPU_EVENT_SET, 1, 0x0ABE, TPU_TRIGGER_RF(12) },
	{ TPU_EVENT_SET, 1, 0x0B92, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_SET) },
	{ TPU_EVENT_SET, 1, 0x0C2A, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_MONON_SET) },
	{ TPU_EVENT_CLEAR, 1, 0x0E3A, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RECEIVE_CLEAR) | TPU_TRIGGER_RF(21) },
	{ TPU_EVENT_CLEAR, 1, 0x0E3C, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_CLEAR) },
	{ TPU_EVENT_SET, 1, 0x1000, TPU_TRIGGER_RF(9) },
	{ TPU_EVENT_SET, 1, 0x10AE, TPU_TRIGGER_RF(13) },
	{ TPU_EVENT_SET, 1, 0x1182, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_SET) },
	{ TPU_EVENT_SET, 1, 0x121A, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_MONON_SET) },
	{ TPU_EVENT_CLEAR, 1, 0x142A, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RECEIVE_CLEAR) | TPU_TRIGGER_RF(21) },
	{ TPU_EVENT_CLEAR, 1, 0x142C, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_CLEAR) },
	{ TPU_EVENT_SET, 1, 0x15F0, TPU_TRIGGER_RF(10) },
	{ TPU_EVENT_SET, 1, 0x169E, TPU_TRIGGER_RF(14) },
	{ TPU_EVENT_SET, 1, 0x1772, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_SET) },
	{ TPU_EVENT_SET, 1, 0x180A, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_MONON_SET) },
	{ TPU_EVENT_CLEAR, 1, 0x1A1A, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RECEIVE_CLEAR) | TPU_TRIGGER_RF(21) },
	{ TPU_EVENT_CLEAR, 1, 0x1A1C, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_CLEAR) },
	{ TPU_EVENT_SET, 15, 0x1A40, TPU_TRIGGER_T_OUT(12) },
	{ TPU_EVENT_SET, 15, 0x1AFE, TPU_TRIGGER_ADCTRIG },
	{ TPU_EVENT_CLEAR, 15, 0x1B08, TPU_TRIGGER_T_OUT(12) },
	{ TPU_EVENT_SET, 15, 0x1BC6, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_INT_GP0) },
	{ TPU_EVENT_SET, 1, 0x1BE0, TPU_TRIGGER_RF(11) },
	{ TPU_EVENT_SET, 1, 0x1C8E, TPU_TRIGGER_RF(15) },
	{ TPU_EVENT_SET, 1, 0x1D62, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_SET) },
	{ TPU_EVENT_SET, 1, 0x1DFA, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_MONON_SET) },
	{ TPU_EVENT_CLEAR, 1, 0x200A, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RECEIVE_CLEAR) | TPU_TRIGGER_RF(21) },
	{ TPU_EVENT_CLEAR, 1, 0x200C, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_CLEAR) },
	{ TPU_EVENT_SET, 31, 0x7FF8, TPU_TRIGGER_T_OUT(0) },
};

/* Multi-frame frequency-correction search window captured from EL71 v45. */
static const struct tpu_event FCCH_EVENTS[] = {
	{ TPU_EVENT_SET, 0, 0x0001, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_FRAME_INT) },
	{ TPU_EVENT_CLEAR, 6, 0x0008, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RECEIVE_CLEAR) | TPU_TRIGGER_RF(21) },
	{ TPU_EVENT_CLEAR, 6, 0x000A, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_CLEAR) },
	{ TPU_EVENT_SET, 1, 0x003C, TPU_TRIGGER_RF(11) },
	{ TPU_EVENT_SET, 1, 0x00EA, TPU_TRIGGER_RF(10) },
	{ TPU_EVENT_SET, 1, 0x01BE, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_SET) },
	{ TPU_EVENT_SET, 1, 0x0256, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_FCON_SET) },
	{ TPU_EVENT_CLEAR, 4, 0x1120, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RECEIVE_CLEAR) | TPU_TRIGGER_RF(21) },
	{ TPU_EVENT_CLEAR, 4, 0x1122, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_CLEAR) },
	{ TPU_EVENT_CLEAR, 8, 0x2E6C, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RECEIVE_CLEAR) | TPU_TRIGGER_RF(21) },
	{ TPU_EVENT_CLEAR, 8, 0x2E6E, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_CLEAR) },
	{ TPU_EVENT_SET, 31, 0x7FF8, TPU_TRIGGER_T_OUT(0) },
};

/* Synchronized receive window captured from EL71 v45. */
static const struct tpu_event SCH_EVENTS[] = {
	{ TPU_EVENT_SET, 0, 0x0001, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_FRAME_INT) },
	{ TPU_EVENT_TIMING_ADVANCE, 10, 0, 0 },
	{ TPU_EVENT_CLEAR, 6, 0x000A, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RECEIVE_CLEAR) },
	{ TPU_EVENT_CLEAR, 1, 0x000C, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RECEIVE_CLEAR) | TPU_TRIGGER_RF(21) },
	{ TPU_EVENT_CLEAR, 1, 0x000E, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_CLEAR) },
	{ TPU_EVENT_SET, 6, 0x0014, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_SCON_SET) },
	{ TPU_EVENT_SET, 1, 0x003C, TPU_TRIGGER_RF(11) },
	{ TPU_EVENT_SET, 1, 0x00EA, TPU_TRIGGER_RF(10) },
	{ TPU_EVENT_SET, 1, 0x01BE, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_SET) },
	{ TPU_EVENT_SET, 1, 0x0256, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_SCON_SET) },
	{ TPU_EVENT_CLEAR, 6, 0x051A, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RECEIVE_CLEAR) | TPU_TRIGGER_RF(26) },
	{ TPU_EVENT_CLEAR, 1, 0x075C, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RECEIVE_CLEAR) | TPU_TRIGGER_RF(21) },
	{ TPU_EVENT_CLEAR, 1, 0x075E, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_RXON_CLEAR) },
	{ TPU_EVENT_TIMING_ADVANCE, 10, 0, 0 },
	{ TPU_EVENT_SET, 31, 0x7FF8, TPU_TRIGGER_T_OUT(0) },
};

static const struct tpu_event TPU_IDLE_EVENTS[] = {
	{ TPU_EVENT_SET, 0, 0x0001, TPU_TRIGGER_DECODER(TPU_TIMER_DECODER_FRAME_INT) },
	{ TPU_EVENT_SET, 31, 0x7FF8, TPU_TRIGGER_T_OUT(0) },
};

static const struct tpu_event RF_POWER_UP_EVENTS[] = {
	{ TPU_EVENT_TIMING_ADVANCE, 31, 0, 0 },
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
static bool l1_initialized;
static bool trx_initialized;
static uint16_t scan_channels[GSM_SCAN_CHANNEL_COUNT];
static uint16_t scan_request_channels[GSM_TRX_MONITOR_COUNT];
static uint16_t scan_request_gain_states[GSM_TRX_MONITOR_COUNT];
static struct channel_scan_result scan_results[GSM_CHANNEL_COUNT];
static struct fcch_result fcch_result;
static struct fcch_result fcch_refinement_result;
static struct sch_result sch_result;
static struct gsm_l1_candidate current_candidate;
static struct gsm_l1_runtime runtime;
static uint32_t fcch_refinement_attempt_count;
static uint32_t sch_attempt_count;
static uint32_t fcch_window;
static bool fcch_detected;
static uint16_t fcch_correction_tick;
static int32_t fcch_reference_afc_frequency_hz;
static int32_t fcch_result_afc_frequency_hz;
static volatile enum fcch_tpu_phase fcch_tpu_phase;
static volatile enum sch_scan_state sch_scan_state;
static enum sch_tpu_phase sch_tpu_phase;

static void poll_runtime(void) {
	if (runtime.poll)
		runtime.poll(runtime.context);
}

static void clear_tpu_ram(void) {
	for (size_t i = 0; i < TPU_RAM_SIZE / sizeof(uint32_t); i++)
		TPU_RAM(i) = 0;
}

static void write_tpu_schedule(const struct tpu_event *events, size_t count, uint32_t bank) {
	uint32_t event_base = bank == 0 ? TPU_TIMER_BANK_0_OFFSET : TPU_TIMER_BANK_1_OFFSET;

	for (size_t i = 0; i < count; i++) {
		uint32_t triggers = events[i].triggers;
		uint16_t rf_trigger = (triggers & TPU_TRIGGER_RF_MASK);
		bool is_rf_event = rf_trigger >= TPU_RF_TELEGRAM_TRIGGER_FIRST &&
			rf_trigger < TPU_RF_TELEGRAM_TRIGGER_FIRST + GSM_TRX_TELEGRAM_COUNT;
		if (is_rf_event)
			triggers += bank * GSM_TRX_TELEGRAM_COUNT;

		uint16_t control = events[i].operation | (events[i].group << TPU_EVENT_GROUP_SHIFT) |
			((triggers >> TPU_EVENT_TRIGGER_HIGH_SHIFT) & TPU_EVENT_TRIGGER_HIGH_MASK);
		uint16_t tick = events[i].tick |
			(((triggers >> TPU_EVENT_TRIGGER_25_SHIFT) & 1) << TPU_EVENT_TIME_TRIGGER_SHIFT);
		uint16_t trigger_low = (triggers & TPU_EVENT_TRIGGER_LOW_MASK);
		uint32_t offset = TPU_TIMER_RAM_BASE + event_base + i * TPU_EVENT_WORD_COUNT;

		TPU_RAM(offset) = control;
		TPU_RAM(offset + 1) = tick;
		TPU_RAM(offset + 2) = trigger_low;
	}
}

static void write_initial_tpu_schedules(void) {
	write_tpu_schedule(RF_POWER_UP_EVENTS, ARRAY_SIZE(RF_POWER_UP_EVENTS), 0);
	write_tpu_schedule(RF_INITIALIZATION_EVENTS, ARRAY_SIZE(RF_INITIALIZATION_EVENTS), 1);
}

static void publish_tpu_event_window(uint32_t bank, size_t event_count, uint32_t event_groups) {
	uint32_t first_event = bank == 0 ? TPU_TIMER_BANK_0_OFFSET : TPU_TIMER_BANK_1_OFFSET;

	TPU_EAPT = first_event + event_count * TPU_EVENT_WORD_COUNT;
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

	return NULL;
}

static uint16_t channel_index_to_arfcn(uint16_t channel_index) {
	const struct gsm_scan_range *range = find_scan_range(channel_index);

	return range->arfcn_first + channel_index - range->table_first;
}

static void exclude_weaker_adjacent_fcch_channel(uint16_t channel_index, int neighbor_offset) {
	const struct gsm_scan_range *range = find_scan_range(channel_index);
	int neighbor_index = channel_index + neighbor_offset;
	if (neighbor_index < range->table_first || neighbor_index >= range->table_first + range->channel_count)
		return;

	const struct channel_scan_result *selected = &scan_results[channel_index];
	struct channel_scan_result *neighbor = &scan_results[neighbor_index];
	if (selected->level_dbm_x16 - neighbor->level_dbm_x16 >= FCCH_ADJACENT_SUPPRESSION_DBM_X16)
		neighbor->fcch_excluded = true;
}

static bool select_next_fcch_candidate(struct gsm_l1_candidate *candidate) {
	uint16_t selected_channel = GSM_CHANNEL_NONE;

	for (size_t i = 0; i < ARRAY_SIZE(scan_channels); i++) {
		uint16_t channel_index = scan_channels[i];
		const struct channel_scan_result *result = &scan_results[channel_index];

		if (result->fcch_excluded)
			continue;
		if (result->measurements == 0)
			continue;
		if (result->level_dbm_x16 < FCCH_MIN_RX_LEVEL_DBM_X16)
			continue;
		if (selected_channel != GSM_CHANNEL_NONE && result->level_dbm_x16 <= scan_results[selected_channel].level_dbm_x16)
			continue;

		selected_channel = channel_index;
	}
	if (selected_channel == GSM_CHANNEL_NONE)
		return false;

	struct channel_scan_result *selected = &scan_results[selected_channel];
	selected->fcch_excluded = true;
	exclude_weaker_adjacent_fcch_channel(selected_channel, -1);
	exclude_weaker_adjacent_fcch_channel(selected_channel, 1);

	const struct gsm_scan_range *range = find_scan_range(selected_channel);
	candidate->band = range->band;
	candidate->channel_index = selected_channel;
	candidate->arfcn = channel_index_to_arfcn(selected_channel);
	candidate->gain_state = selected->gain_state;
	candidate->rx_level_dbm_x16 = selected->level_dbm_x16;
	candidate->afc_frequency_hz = gsm_board_get_frequency_correction_hz();
	return true;
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

	TPU_GSMCLK1 = (1 << TPU_GSMCLK1_K_SHIFT);
	TPU_GSMCLK2 = (4 << TPU_GSMCLK2_L_SHIFT);
	TPU_GSMCLK3 = TPU_GSMCLK3_INIT;
	TPU_OVERFLOW = TPU_FRAME_TICKS - 1;
	TPU_OFFSET = 0;
	TPU_INT(0) = 0;
	TPU_SRC(0) = (MOD_SRC_CLRR | MOD_SRC_SRE);
	VIC_CON(VIC_TPU_INT0_IRQ) = 1;

	tpu_loop_phase = TPU_LOOP_RF_POWER_UP;
	tpu_schedule_bank = 0;
	tpu_rf_transition_frame = 0;
	rf_initialization_complete = false;
	publish_tpu_event_window(0, ARRAY_SIZE(RF_POWER_UP_EVENTS), TPU_RF_POWER_UP_EVENT_GROUPS);

	cpu_enable_irq(true);
	TPU_PARAM = (TPU_PARAM_TINI | TPU_PARAM_FDIS);
	tpu_loop_running = true;
}

static void tpu_loop_stop(void) {
	if (!tpu_loop_running)
		return;

	TPU_PARAM = 0;
	VIC_CON(VIC_TPU_INT0_IRQ) = 0;
	TPU_SRC(0) = MOD_SRC_CLRR;
	tpu_loop_running = false;
}

bool gsm_l1_trx_init(void) {
	if (!l1_initialized)
		return false;

	gsm_board_enable_transceiver_power();
	gsm_trx_init();
	tpu_loop_start();

	stopwatch_t start = stopwatch_get();
	while (!rf_initialization_complete && stopwatch_elapsed_ms(start) < runtime.command_timeout_ms)
		poll_runtime();
	if (!rf_initialization_complete)
		return false;
	if (!gsm_trx_is_idle())
		return false;

	gsm_board_set_frequency_correction_hz(gsm_board_get_frequency_correction_hz());
	trx_initialized = true;
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
	for (size_t i = 0; i < DSP_FCCH_RESULT_WORD_COUNT; i++)
		dsp_hw_shared_memory[DSP_FCCH_STATUS_OFFSET + i] = DSP_FCCH_SENTINEL_BASE + i;
}

static void configure_acquisition_receiver(uint32_t bank) {
	gsm_trx_configure_acquisition(bank, current_candidate.channel_index, current_candidate.gain_state);
}

static uint16_t fcch_start_to_tpu_phase_tick(uint16_t start) {
	int32_t phase_tick = (start % GSM_BITS_PER_FRAME) * TPU_TICKS_PER_GSM_BIT - FCCH_PHASE_ADVANCE_TICKS;

	if (phase_tick < 0)
		phase_tick += TPU_FRAME_TICKS;
	return (uint16_t) phase_tick;
}

static void read_fcch_result(struct fcch_result *result) {
	result->communication_flags = (DSP_COM_STATUS & DSP_FCCH_COMMUNICATION_FLAGS);
	result->status = dsp_hw_shared_memory[DSP_FCCH_STATUS_OFFSET];
	result->start = dsp_hw_shared_memory[DSP_FCCH_START_OFFSET];
	result->quality = dsp_hw_shared_memory[DSP_FCCH_QUALITY_OFFSET];
	result->rms = (int16_t) dsp_hw_shared_memory[DSP_FCCH_RMS_OFFSET];
	result->frequency = (int16_t) dsp_hw_shared_memory[DSP_FCCH_FREQUENCY_OFFSET];
}

static bool is_fcch_result_valid(const struct fcch_result *result) {
	return result->communication_flags == 0 && result->status == 2;
}

static void collect_fcch_result(void) {
	read_fcch_result(&fcch_result);
	fcch_window++;
	if (is_fcch_result_valid(&fcch_result)) {
		fcch_detected = true;
		fcch_correction_tick = fcch_start_to_tpu_phase_tick(fcch_result.start);
		fcch_result_afc_frequency_hz = fcch_reference_afc_frequency_hz + fcch_result.frequency;
		fcch_scan_state = FCCH_SCAN_COMPLETE;
		return;
	}

	if (fcch_window != FCCH_SEARCH_WINDOW_COUNT)
		return;

	fcch_scan_state = FCCH_SCAN_COMPLETE;
}

static void collect_fcch_refinement_result(void) {
	read_fcch_result(&fcch_refinement_result);
	fcch_refinement_attempt_count++;
	if (is_fcch_result_valid(&fcch_refinement_result)) {
		gsm_board_set_frequency_correction_hz(gsm_board_get_frequency_correction_hz() + fcch_refinement_result.frequency);
		fcch_correction_tick = fcch_start_to_tpu_phase_tick(fcch_refinement_result.start);
		TPU_CORRECTION = fcch_correction_tick - 1;
		fcch_scan_state = FCCH_SCAN_REFINEMENT_COMPLETE;
		return;
	}

	if (fcch_refinement_attempt_count == FCCH_REFINEMENT_ATTEMPT_COUNT) {
		fcch_scan_state = FCCH_SCAN_REFINEMENT_FAILED;
		return;
	}

	fcch_scan_state = FCCH_SCAN_REFINEMENT_COMMAND_REQUESTED;
}

static void reset_monitor_results(void) {
	for (size_t i = 0; i < DSP_MON_VALUE_COUNT; i++)
		dsp_hw_shared_memory[DSP_MON_VALUES_OFFSET + i] = 0xFFFF;
}

static void collect_monitor_results(const uint16_t *channels) {
	for (size_t i = 0; i < GSM_TRX_MONITOR_COUNT; i++) {
		uint16_t raw = dsp_hw_shared_memory[DSP_MON_VALUES_OFFSET + DSP_MON_RESULT_FIRST + i];
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
		dsp_hw_shared_memory[DSP_MON_VALUES_OFFSET + DSP_MON_RESULT_FIRST + i] = (raw | 0x8000);
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
	switch (receiver_scan_state) {
		case RECEIVER_SCAN_ACTIVE:
			if (!scan_request_repeated) {
				scan_request_repeated = true;
				return true;
			}
			collect_monitor_results(scan_request_channels);
			scan_request_repeated = false;
			break;

		case RECEIVER_SCAN_REQUESTED:
			receiver_scan_state = RECEIVER_SCAN_ACTIVE;
			scan_pass = 0;
			scan_request_repeated = false;
			channel_scan_start_pass();
			break;

		case RECEIVER_SCAN_IDLE:
		case RECEIVER_SCAN_COMPLETE:
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
	configure_acquisition_receiver(bank);
	write_tpu_schedule(FCCH_EVENTS, ARRAY_SIZE(FCCH_EVENTS), bank);
	publish_tpu_event_window(bank, ARRAY_SIZE(FCCH_EVENTS), event_groups);
}

static void reset_sch_result(void) {
	for (size_t i = 0; i < DSP_SCH_RESULT_WORD_COUNT; i++)
		dsp_hw_shared_memory[DSP_SCH_METRIC_OFFSET + i] = DSP_SCH_SENTINEL_BASE + i;
}

static void publish_sch_frame(uint32_t bank) {
	TPU_CORRECTION = fcch_correction_tick - 1;
	configure_acquisition_receiver(bank);
	reset_sch_result();
	write_tpu_schedule(SCH_EVENTS, ARRAY_SIZE(SCH_EVENTS), bank);
	publish_tpu_event_window(bank, ARRAY_SIZE(SCH_EVENTS), TPU_SCH_EVENT_GROUPS);
}

static bool collect_sch_result(void) {
	sch_result.communication_flags = (DSP_COM_STATUS & DSP_SCH_COMMUNICATION_FLAG);
	sch_result.metric = dsp_hw_shared_memory[DSP_SCH_METRIC_OFFSET];
	sch_result.status = dsp_hw_shared_memory[DSP_SCH_STATUS_OFFSET];
	sch_result.data[0] = dsp_hw_shared_memory[DSP_SCH_DATA_OFFSET];
	sch_result.data[1] = dsp_hw_shared_memory[DSP_SCH_DATA_OFFSET + 1];
	sch_result.equalizer_position = (int16_t) dsp_hw_shared_memory[DSP_SCH_EQUALIZER_POSITION_OFFSET];
	sch_attempt_count++;
	if (sch_result.communication_flags == 0 && sch_result.status == 0)
		return true;

	return sch_attempt_count == SCH_ATTEMPT_COUNT;
}

static void sch_scan_start(uint32_t bank) {
	tpu_loop_phase = TPU_LOOP_SCH;
	sch_tpu_phase = SCH_TPU_RECEIVE;
	publish_sch_frame(bank);
}

static void sch_candidate_start(uint32_t bank) {
	fcch_refinement_attempt_count = 0;
	sch_attempt_count = 0;
	gsm_board_set_frequency_correction_hz(fcch_result_afc_frequency_hz);
	TPU_CORRECTION = fcch_correction_tick - 1;

	sch_scan_state = SCH_SCAN_ACTIVE;
	tpu_loop_phase = TPU_LOOP_FCCH;
	fcch_scan_state = FCCH_SCAN_REFINEMENT_BB_OFF_REQUESTED;
	fcch_tpu_phase = FCCH_TPU_RF_QUIET_FIRST;
	gsm_trx_reset_telegram_table();
	gsm_trx_write_telegram_table(0);
	gsm_trx_write_telegram_table(1);
	publish_rf_initialization_frame(bank, TPU_RF_QUIET_EVENT_GROUPS);
}

static void fcch_scan_start(uint32_t bank) {
	tpu_loop_phase = TPU_LOOP_FCCH;
	fcch_scan_state = FCCH_SCAN_BB_OFF_REQUESTED;
	fcch_window = 0;
	fcch_correction_tick = 0;
	fcch_reference_afc_frequency_hz = gsm_board_get_frequency_correction_hz();
	fcch_tpu_phase = FCCH_TPU_RF_QUIET_FIRST;

	gsm_trx_reset_telegram_table();
	gsm_trx_write_telegram_table(0);
	gsm_trx_write_telegram_table(1);
	publish_rf_initialization_frame(bank, TPU_RF_QUIET_EVENT_GROUPS);
}

static void publish_idle_frame(uint32_t bank) {
	write_tpu_schedule(TPU_IDLE_EVENTS, ARRAY_SIZE(TPU_IDLE_EVENTS), bank);
	publish_tpu_event_window(bank, ARRAY_SIZE(TPU_IDLE_EVENTS), TPU_SEARCH_EVENT_GROUPS);
}

static void sch_scan_complete(uint32_t bank) {
	sch_scan_state = SCH_SCAN_COMPLETE;
	tpu_loop_phase = TPU_LOOP_COMPLETE;
	publish_idle_frame(bank);
}

static void publish_next_search_frame(uint32_t bank, uint32_t event_groups) {
	if (channel_scan_advance()) {
		configure_monitoring_frame(bank, scan_request_channels);
		write_tpu_schedule(SIGNAL_SEARCH_EVENTS, ARRAY_SIZE(SIGNAL_SEARCH_EVENTS), bank);
		dsp_hw_shared_memory[DSP_MON_INDEX_OFFSET] = DSP_MON_RESULT_FIRST;
		publish_tpu_event_window(bank, ARRAY_SIZE(SIGNAL_SEARCH_EVENTS), event_groups);
		return;
	}

	if (receiver_scan_state == RECEIVER_SCAN_COMPLETE && fcch_scan_state == FCCH_SCAN_REQUESTED) {
		fcch_scan_start(bank);
		return;
	}

	publish_idle_frame(bank);
}

static void publish_next_fcch_frame(uint32_t bank) {
	switch (fcch_tpu_phase) {
		case FCCH_TPU_REFINEMENT_PREPARE:
			if (fcch_scan_state == FCCH_SCAN_REFINEMENT_COMMAND_REQUESTED) {
				publish_fcch_frame(bank, TPU_FCCH_HOLD_EVENT_GROUPS);
				return;
			}

			fcch_tpu_phase = FCCH_TPU_REFINEMENT_START;
			publish_fcch_frame(bank, TPU_FCCH_START_EVENT_GROUPS);
			return;

		case FCCH_TPU_REFINEMENT_START:
			fcch_tpu_phase = FCCH_TPU_REFINEMENT_WAIT_RESULT;
			publish_fcch_frame(bank, TPU_FCCH_HOLD_EVENT_GROUPS);
			return;

		case FCCH_TPU_REFINEMENT_WAIT_RESULT:
			if ((DSP_COM_STATUS & DSP_FCCH_COMMUNICATION_FLAGS) != 0) {
				publish_fcch_frame(bank, TPU_FCCH_HOLD_EVENT_GROUPS);
				return;
			}

			collect_fcch_refinement_result();
			switch (fcch_scan_state) {
				case FCCH_SCAN_REFINEMENT_COMPLETE:
					sch_scan_start(bank);
					break;

				case FCCH_SCAN_REFINEMENT_FAILED:
					sch_scan_complete(bank);
					break;

				default:
					fcch_tpu_phase = FCCH_TPU_REFINEMENT_PREPARE;
					publish_fcch_frame(bank, TPU_FCCH_HOLD_EVENT_GROUPS);
					break;
			}
			return;

		case FCCH_TPU_RF_QUIET_FIRST: {
			bool baseband_off_requested = fcch_scan_state == FCCH_SCAN_BB_OFF_REQUESTED ||
				fcch_scan_state == FCCH_SCAN_REFINEMENT_BB_OFF_REQUESTED;
			if (baseband_off_requested) {
				publish_rf_initialization_frame(bank, TPU_RF_QUIET_EVENT_GROUPS);
				return;
			}

			fcch_tpu_phase = FCCH_TPU_RF_QUIET_SECOND;
			publish_rf_initialization_frame(bank, TPU_RF_QUIET_EVENT_GROUPS);
			return;
		}

		case FCCH_TPU_RF_QUIET_SECOND:
			fcch_tpu_phase = FCCH_TPU_RF_INITIALIZATION;
			publish_rf_initialization_frame(bank, TPU_INITIALIZATION_EVENT_GROUPS);
			return;

		case FCCH_TPU_RF_INITIALIZATION:
			if (sch_scan_state == SCH_SCAN_ACTIVE) {
				fcch_tpu_phase = FCCH_TPU_REFINEMENT_PREPARE;
				publish_fcch_frame(bank, TPU_FCCH_HOLD_EVENT_GROUPS);
				return;
			}

			fcch_tpu_phase = FCCH_TPU_PREPARE_FIRST;
			publish_fcch_frame(bank, TPU_FCCH_PREPARE_EVENT_GROUPS);
			return;

		case FCCH_TPU_PREPARE_FIRST:
			fcch_tpu_phase = FCCH_TPU_PREPARE_SECOND;
			fcch_scan_state = FCCH_SCAN_COMMAND_REQUESTED;
			publish_fcch_frame(bank, TPU_FCCH_PREPARE_EVENT_GROUPS);
			return;

		case FCCH_TPU_PREPARE_SECOND:
			if (fcch_scan_state == FCCH_SCAN_COMMAND_REQUESTED) {
				publish_fcch_frame(bank, TPU_FCCH_HOLD_EVENT_GROUPS);
				return;
			}

			fcch_tpu_phase = FCCH_TPU_START;
			publish_fcch_frame(bank, TPU_FCCH_START_EVENT_GROUPS);
			return;

		case FCCH_TPU_START:
			fcch_tpu_phase = FCCH_TPU_HOLD_FIRST;
			publish_fcch_frame(bank, TPU_FCCH_HOLD_EVENT_GROUPS);
			return;

		case FCCH_TPU_HOLD_FIRST:
			fcch_tpu_phase = FCCH_TPU_HOLD_SECOND;
			publish_fcch_frame(bank, TPU_FCCH_HOLD_EVENT_GROUPS);
			return;

		case FCCH_TPU_HOLD_SECOND:
			fcch_tpu_phase = FCCH_TPU_FINISH;
			publish_fcch_frame(bank, TPU_FCCH_PREPARE_EVENT_GROUPS);
			return;

		case FCCH_TPU_FINISH:
			if ((DSP_COM_STATUS & DSP_FCCH_COMMUNICATION_FLAGS) != 0) {
				publish_fcch_frame(bank, TPU_FCCH_HOLD_EVENT_GROUPS);
				return;
			}

			collect_fcch_result();
			switch (fcch_scan_state) {
				case FCCH_SCAN_COMPLETE:
					tpu_loop_phase = TPU_LOOP_COMPLETE;
					publish_idle_frame(bank);
					break;

				case FCCH_SCAN_BB_OFF_REQUESTED:
					fcch_tpu_phase = FCCH_TPU_RF_QUIET_FIRST;
					gsm_trx_reset_telegram_table();
					gsm_trx_write_telegram_table(0);
					gsm_trx_write_telegram_table(1);
					publish_rf_initialization_frame(bank, TPU_RF_QUIET_EVENT_GROUPS);
					break;

				default:
					fcch_tpu_phase = FCCH_TPU_PREPARE_SECOND;
					fcch_scan_state = FCCH_SCAN_COMMAND_REQUESTED;
					publish_fcch_frame(bank, TPU_FCCH_PREPARE_EVENT_GROUPS);
					break;
			}
			return;
	}
}

static void publish_next_sch_frame(uint32_t bank) {
	switch (sch_tpu_phase) {
		case SCH_TPU_RECEIVE:
			if ((DSP_COM_STATUS & DSP_SCH_COMMUNICATION_FLAG) != 0) {
				publish_idle_frame(bank);
				return;
			}

			TPU_CORRECTION = TPU_SCH_RESULT_BOUNDARY_TICK - 1;
			sch_tpu_phase = SCH_TPU_RESULT_BOUNDARY;
			publish_idle_frame(bank);
			return;

		case SCH_TPU_RESULT_BOUNDARY:
			if (!collect_sch_result()) {
				tpu_loop_phase = TPU_LOOP_FCCH;
				fcch_scan_state = FCCH_SCAN_REFINEMENT_COMMAND_REQUESTED;
				fcch_tpu_phase = FCCH_TPU_REFINEMENT_PREPARE;
				publish_fcch_frame(bank, TPU_FCCH_HOLD_EVENT_GROUPS);
				return;
			}
			sch_scan_complete(bank);
			return;
	}
}

static void publish_next_tpu_frame(void) {
	bool repeat_transition_bank = tpu_loop_phase == TPU_LOOP_READY && tpu_rf_transition_frame == 1;

	if (!repeat_transition_bank)
		tpu_schedule_bank ^= 1;

	switch (tpu_loop_phase) {
		case TPU_LOOP_RF_POWER_UP:
			tpu_loop_phase = TPU_LOOP_RF_INITIALIZATION;
			publish_rf_initialization_frame(tpu_schedule_bank, TPU_INITIALIZATION_EVENT_GROUPS);
			return;

		case TPU_LOOP_READY:
			if (publish_rf_transition_frame(tpu_schedule_bank))
				return;
			if (receiver_scan_state == RECEIVER_SCAN_REQUESTED) {
				tpu_loop_phase = TPU_LOOP_SEARCH;
				publish_next_search_frame(tpu_schedule_bank, TPU_FIRST_SEARCH_EVENT_GROUPS);
				return;
			}
			break;

		case TPU_LOOP_SEARCH:
			publish_next_search_frame(tpu_schedule_bank, TPU_SEARCH_EVENT_GROUPS);
			return;

		case TPU_LOOP_FCCH:
			publish_next_fcch_frame(tpu_schedule_bank);
			return;

		case TPU_LOOP_SCH:
			publish_next_sch_frame(tpu_schedule_bank);
			return;

		case TPU_LOOP_COMPLETE:
			if (sch_scan_state == SCH_SCAN_REQUESTED) {
				sch_candidate_start(tpu_schedule_bank);
				return;
			}
			break;

		case TPU_LOOP_RF_INITIALIZATION:
			break;
	}

	publish_idle_frame(tpu_schedule_bank);
}

static bool wait_for_dsp_communication(uint16_t flags) {
	stopwatch_t start = stopwatch_get();
	while ((DSP_COM_STATUS & flags) != 0 && stopwatch_elapsed_ms(start) < runtime.command_timeout_ms)
		poll_runtime();

	return (DSP_COM_STATUS & flags) == 0;
}

static void signal_dsp_communication(uint16_t flags) {
	DSP_COM_SET = flags;
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;
}

static void dsp_runtime_command_start(uint16_t command, const uint16_t *parameters, size_t count) {
	dsp_hw_shared_memory[DSP_RUNTIME_PIPE_OFFSET] = command;
	for (size_t i = 0; i < count; i++)
		dsp_hw_shared_memory[DSP_RUNTIME_PIPE_OFFSET + 1 + i] = parameters[i];

	signal_dsp_communication(BIT(0));
}

static bool dsp_runtime_command_execute(uint16_t command, const uint16_t *parameters, size_t count) {
	dsp_runtime_command_start(command, parameters, count);
	return wait_for_dsp_communication(BIT(0));
}

static bool dsp_execute_baseband_off(void) {
	uint16_t sequence = (dsp_hw_shared_memory[DSP_BB_OFF_SEQUENCE_OFFSET] & 0xFF00);
	sequence++;
	dsp_hw_shared_memory[DSP_BB_OFF_SEQUENCE_OFFSET] = sequence;
	return dsp_runtime_command_execute(DSP_RUNTIME_COMMAND_BB_OFF, NULL, 0);
}

static void dsp_fcch_command_start(void) {
	/* EL71 v45 search-mode payload from dsp_fc_init(0, 0x21, 0x73). */
	static const uint16_t PARAMETERS[] = { 1, 115, 33 };

	reset_fcch_result();
	dsp_runtime_command_start(DSP_RUNTIME_COMMAND_FC_INIT, PARAMETERS, ARRAY_SIZE(PARAMETERS));
}

static void dsp_fcch_refinement_command_start(void) {
	/* EL71 v45 synchronization payload from dsp_fc_init(1, 0x23, 0x73). */
	static const uint16_t PARAMETERS[] = { 0, 115, 35 };

	reset_fcch_result();
	dsp_runtime_command_start(DSP_RUNTIME_COMMAND_FC_INIT, PARAMETERS, ARRAY_SIZE(PARAMETERS));
}

static bool execute_requested_fcch_command(void) {
	switch (fcch_scan_state) {
		case FCCH_SCAN_BB_OFF_REQUESTED:
			if (!dsp_execute_baseband_off())
				return false;
			fcch_scan_state = FCCH_SCAN_ACTIVE;
			break;

		case FCCH_SCAN_REFINEMENT_BB_OFF_REQUESTED:
			if (!dsp_execute_baseband_off())
				return false;
			dsp_fcch_refinement_command_start();
			fcch_scan_state = FCCH_SCAN_REFINEMENT_ACTIVE;
			break;

		case FCCH_SCAN_COMMAND_REQUESTED:
			dsp_fcch_command_start();
			fcch_scan_state = FCCH_SCAN_ACTIVE;
			break;

		case FCCH_SCAN_REFINEMENT_COMMAND_REQUESTED:
			dsp_fcch_refinement_command_start();
			fcch_scan_state = FCCH_SCAN_REFINEMENT_ACTIVE;
			break;

		case FCCH_SCAN_IDLE:
		case FCCH_SCAN_REQUESTED:
		case FCCH_SCAN_ACTIVE:
		case FCCH_SCAN_REFINEMENT_ACTIVE:
		case FCCH_SCAN_REFINEMENT_COMPLETE:
		case FCCH_SCAN_REFINEMENT_FAILED:
		case FCCH_SCAN_COMPLETE:
			break;
	}

	return true;
}

static bool dsp_channel_decoder_profiles_init(void) {
	/* Exact command payloads from EL71 v45 dsp_channel_decoder_init_profiles at 0xA0A748C8. */
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
		if (!dsp_runtime_command_execute(DSP_RUNTIME_COMMAND_DEC_INIT, PROFILES[i].parameters, PROFILES[i].count))
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
	if (!dsp_hw_load_container(DSP_EL71_V45_CONTAINER, sizeof(DSP_EL71_V45_CONTAINER)))
		return false;
	stopwatch_usleep_wd(1000);

	if (!dsp_channel_decoder_profiles_init())
		return false;
	if (!dsp_runtime_command_execute(DSP_RUNTIME_COMMAND_MODU_INIT, MODU_INIT_PARAMETERS, ARRAY_SIZE(MODU_INIT_PARAMETERS)))
		return false;
	dsp_hw_shared_memory[DSP_RF_ADAPT_EXTENSION_OFFSET] = DSP_RF_ADAPT_EXTENSION_VALUE;
	if (!dsp_runtime_command_execute(DSP_RUNTIME_COMMAND_RF_ADAPT, RF_ADAPT_PARAMETERS, ARRAY_SIZE(RF_ADAPT_PARAMETERS)))
		return false;
	dsp_hw_shared_memory[DSP_RUNTIME_CONFIG_OFFSET] = DSP_RUNTIME_CONFIG_VALUE;
	if (!dsp_runtime_command_execute(DSP_RUNTIME_COMMAND_IDLE, NULL, 0))
		return false;
	if (!dsp_runtime_command_execute(DSP_RUNTIME_COMMAND_IQ_SWAP_1, IQ_SWAP_PARAMETERS, ARRAY_SIZE(IQ_SWAP_PARAMETERS)))
		return false;
	if (!dsp_runtime_command_execute(DSP_RUNTIME_COMMAND_IQ_SWAP_2, IQ_SWAP_PARAMETERS, ARRAY_SIZE(IQ_SWAP_PARAMETERS)))
		return false;
	if (!dsp_runtime_command_execute(DSP_RUNTIME_COMMAND_MODU_INIT, MODU_INIT_PARAMETERS, ARRAY_SIZE(MODU_INIT_PARAMETERS)))
		return false;
	if (!dsp_runtime_command_execute(DSP_RUNTIME_COMMAND_IDLE, NULL, 0))
		return false;

	dsp_hw_shared_memory[DSP_MON_INDEX_OFFSET] = 0;
	reset_monitor_results();
	return true;
}

static void build_channel_scan_order(void) {
	size_t position = 0;

	for (size_t range_index = 0; range_index < ARRAY_SIZE(GSM_SCAN_RANGES); range_index++) {
		const struct gsm_scan_range *range = &GSM_SCAN_RANGES[range_index];

		for (uint16_t offset = 0; offset < range->channel_count; offset++)
			scan_channels[position++] = range->table_first + offset;
	}
}

bool gsm_l1_scan_channels(struct gsm_l1_candidate *candidates, size_t capacity, size_t *count, uint32_t timeout_ms) {
	if (!trx_initialized)
		return false;
	if (candidates == NULL)
		return false;
	if (count == NULL)
		return false;
	if (capacity == 0)
		return false;

	*count = 0;
	sch_scan_state = SCH_SCAN_IDLE;
	for (size_t i = 0; i < ARRAY_SIZE(scan_channels); i++) {
		struct channel_scan_result *result = &scan_results[scan_channels[i]];

		result->level_dbm_x16 = 0;
		result->measurements = 0;
		result->gain_state = gsm_board_get_initial_gain_state();
		result->fcch_excluded = false;
	}

	receiver_scan_state = RECEIVER_SCAN_REQUESTED;
	stopwatch_t start = stopwatch_get();
	while (receiver_scan_state != RECEIVER_SCAN_COMPLETE && stopwatch_elapsed_ms(start) < timeout_ms)
		poll_runtime();

	for (size_t i = 0; i < ARRAY_SIZE(scan_channels); i++) {
		uint16_t channel_index = scan_channels[i];
		const struct channel_scan_result *channel_result = &scan_results[channel_index];

		if (channel_result->measurements == 0)
			return false;
	}

	while (*count < capacity && select_next_fcch_candidate(&candidates[*count]))
		(*count)++;
	fcch_scan_state = FCCH_SCAN_IDLE;
	return true;
}

bool gsm_l1_search_fcch(const struct gsm_l1_candidate *candidate, struct gsm_l1_fcch_result *result, uint32_t timeout_ms) {
	if (!trx_initialized)
		return false;
	if (candidate == NULL)
		return false;
	if (result == NULL)
		return false;
	if (find_scan_range(candidate->channel_index) == NULL)
		return false;

	*result = (struct gsm_l1_fcch_result) { 0 };
	current_candidate = *candidate;
	fcch_detected = false;
	fcch_window = 0;
	fcch_result_afc_frequency_hz = 0;
	fcch_scan_state = FCCH_SCAN_REQUESTED;
	tpu_loop_phase = TPU_LOOP_SEARCH;
	gsm_board_set_frequency_correction_hz(candidate->afc_frequency_hz);
	signal_dsp_communication(DSP_FCCH_PREPARE_COMMUNICATION_FLAG);

	stopwatch_t start = stopwatch_get();
	while (fcch_scan_state != FCCH_SCAN_COMPLETE && stopwatch_elapsed_ms(start) < timeout_ms) {
		if (!execute_requested_fcch_command())
			return false;
		poll_runtime();
	}
	if (fcch_scan_state != FCCH_SCAN_COMPLETE)
		return false;

	result->detected = fcch_detected;
	result->attempt_count = fcch_window;
	if (fcch_window == 0)
		return true;

	result->status = fcch_result.status;
	result->start = fcch_result.start;
	result->quality = fcch_result.quality;
	result->rms = fcch_result.rms;
	result->frequency = fcch_result.frequency;
	result->communication_flags = fcch_result.communication_flags;
	result->phase_tick = fcch_correction_tick;
	result->afc_frequency_hz = fcch_result_afc_frequency_hz;
	return true;
}

static uint32_t sch_data_to_frame_number(uint32_t data) {
	uint32_t t1 = ((data >> 23) & 1) | ((data >> 7) & 0x1FE) | ((data << 9) & 0x600);
	uint32_t t2 = ((data >> 18) & 0x1F);
	uint32_t t3_prime = ((data >> 24) & 1) | ((data >> 15) & 6);
	uint32_t t3 = t3_prime * 10 + 1;
	uint32_t frame_modulo = t3 < t2 ? t3 + 26 - t2 : (t3 - t2) % 26;

	return t1 * 51 * 26 + frame_modulo * 51 + t3;
}

bool gsm_l1_decode_sch(
	const struct gsm_l1_candidate *candidate,
	const struct gsm_l1_fcch_result *fcch,
	struct gsm_l1_sch_result *result,
	uint32_t timeout_ms
) {
	if (!trx_initialized)
		return false;
	if (candidate == NULL)
		return false;
	if (fcch == NULL)
		return false;
	if (result == NULL)
		return false;
	if (!fcch->detected)
		return false;
	if (find_scan_range(candidate->channel_index) == NULL)
		return false;

	*result = (struct gsm_l1_sch_result) { 0 };
	current_candidate = *candidate;
	fcch_correction_tick = fcch->phase_tick;
	fcch_result_afc_frequency_hz = fcch->afc_frequency_hz;
	sch_scan_state = SCH_SCAN_REQUESTED;
	tpu_loop_phase = TPU_LOOP_COMPLETE;
	stopwatch_t start = stopwatch_get();

	while (sch_scan_state != SCH_SCAN_COMPLETE && stopwatch_elapsed_ms(start) < timeout_ms) {
		if (!execute_requested_fcch_command())
			return false;
		poll_runtime();
	}
	if (sch_scan_state != SCH_SCAN_COMPLETE)
		return false;

	result->refinement_attempt_count = fcch_refinement_attempt_count;
	result->attempt_count = sch_attempt_count;
	if (sch_attempt_count != 0 && sch_result.communication_flags == 0 && sch_result.status == 0) {
		result->decoded = true;
		result->metric = sch_result.metric;
		result->status = sch_result.status;
		result->data = sch_result.data[0] | ((uint32_t) sch_result.data[1] << 16);
		result->equalizer_position = sch_result.equalizer_position;
		result->communication_flags = sch_result.communication_flags;
		result->bsic = ((result->data >> 2) & 0x3F);
		result->frame_number = sch_data_to_frame_number(result->data);
	}
	return true;
}

bool gsm_l1_baseband_init(const struct gsm_l1_runtime *runtime_config) {
	if (runtime_config == NULL)
		return false;
	if (runtime_config->command_timeout_ms == 0)
		return false;

	l1_initialized = false;
	trx_initialized = false;
	runtime = *runtime_config;
	build_channel_scan_order();
	if (!gsm_board_init())
		return false;

	dsp_clock_enabled = true;
	clear_tpu_ram();
	if (!dsp_receiver_init())
		return false;

	l1_initialized = true;
	return true;
}

void gsm_l1_deinit(void) {
	tpu_loop_stop();
	if (dsp_clock_enabled) {
		DSP_COM_CLEAR = 0xFFFF;
		(void) dsp_hw_reset();
	}

	dsp_clock_enabled = false;
	l1_initialized = false;
	trx_initialized = false;
	runtime = (struct gsm_l1_runtime) { 0 };
}

void gsm_l1_tpu_irq(void) {
	TPU_SRC(0) |= MOD_SRC_CLRR;
	if (tpu_loop_phase == TPU_LOOP_RF_INITIALIZATION) {
		rf_initialization_complete = true;
		tpu_loop_phase = TPU_LOOP_READY;
	}
	publish_next_tpu_frame();
}
