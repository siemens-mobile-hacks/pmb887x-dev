#include <pmb887x.h>
#include <stopwatch.h>
#include <wdt.h>

#include "gsm-l1/gsm-l1.h"
#include "test.h"

#define CHANNEL_SCAN_TIMEOUT_MS 60000
#define L1_COMMAND_TIMEOUT_MS 100
#define FCCH_CANDIDATE_TIMEOUT_MS 1000
#define SCH_CANDIDATE_TIMEOUT_MS 40000
#define GSM_SCAN_CANDIDATE_COUNT 128
#define MON_RAW_VALUES_PER_DB 16

static struct gsm_l1_candidate candidates[GSM_SCAN_CANDIDATE_COUNT];
static struct gsm_l1_fcch_result fcch_results[GSM_SCAN_CANDIDATE_COUNT];
static size_t candidate_count;

static const char *gsm_band_get_name(enum gsm_band band) {
	switch (band) {
		case GSM_BAND_900:
			return "GSM900";

		case GSM_BAND_DCS1800:
			return "DCS1800";

		case GSM_BAND_PCS1900:
			return "PCS1900";

		case GSM_BAND_850:
			return "GSM850";

		case GSM_BAND_COUNT:
			return "unknown";
	}

	return "unknown";
}

static int32_t round_dbm_x16(int32_t dbm_x16) {
	if (dbm_x16 < 0)
		return -((-dbm_x16 + MON_RAW_VALUES_PER_DB / 2) / MON_RAW_VALUES_PER_DB);
	return (dbm_x16 + MON_RAW_VALUES_PER_DB / 2) / MON_RAW_VALUES_PER_DB;
}

static void poll_l1_runtime(void *context) {
	(void) context;

	test_watchdog_serve();
}

static bool scan_channels(void) {
	wdt_set_max_execution_time(CHANNEL_SCAN_TIMEOUT_MS);
	if (!gsm_l1_scan_channels(candidates, ARRAY_SIZE(candidates), &candidate_count, CHANNEL_SCAN_TIMEOUT_MS))
		return false;

	printf("# GSM_SCAN,candidates=%u\n", (uint32_t) candidate_count);
	return true;
}

static bool search_fcch(void) {
	uint32_t detected_candidates = 0;
	uint32_t windows = 0;

	for (size_t i = 0; i < candidate_count; i++) {
		const struct gsm_l1_candidate *candidate = &candidates[i];
		struct gsm_l1_fcch_result *result = &fcch_results[i];

		wdt_set_max_execution_time(FCCH_CANDIDATE_TIMEOUT_MS);
		if (!gsm_l1_search_fcch(candidate, result, FCCH_CANDIDATE_TIMEOUT_MS))
			return false;
		windows += result->attempt_count;
		if (!result->detected)
			continue;

		detected_candidates++;
		printf("# FCCH,candidate=%u,band=%s,arfcn=%u,rx_level_dbm=%d,windows=%u,status=%u,start=%u,"
			"quality=%u,rms=%d,frequency=%d,phase_tick=%u,afc_frequency=%d,com=%04X\n", (uint32_t) i + 1,
			gsm_band_get_name(candidate->band), (uint32_t) candidate->arfcn,
			round_dbm_x16(candidate->rx_level_dbm_x16), result->attempt_count, (uint32_t) result->status,
			(uint32_t) result->start, (uint32_t) result->quality, (int32_t) result->rms,
			(int32_t) result->frequency, (uint32_t) result->phase_tick, result->afc_frequency_hz,
			(uint32_t) result->communication_flags);
	}

	printf("# FCCH_STATS,candidates=%u,windows=%u,detected_candidates=%u\n",
		(uint32_t) candidate_count, windows, detected_candidates);
	return true;
}

static bool decode_sch(void) {
	uint32_t attempted_candidates = 0;
	uint32_t decoded_candidates = 0;

	for (size_t i = 0; i < candidate_count; i++) {
		const struct gsm_l1_candidate *candidate = &candidates[i];
		const struct gsm_l1_fcch_result *fcch = &fcch_results[i];
		if (!fcch->detected)
			continue;

		attempted_candidates++;
		struct gsm_l1_sch_result result;
		wdt_set_max_execution_time(SCH_CANDIDATE_TIMEOUT_MS);
		if (!gsm_l1_decode_sch(candidate, fcch, &result, SCH_CANDIDATE_TIMEOUT_MS))
			return false;
		if (!result.decoded)
			continue;

		decoded_candidates++;
		printf("# SCH,candidate=%u,band=%s,arfcn=%u,attempts=%u,refinement_attempts=%u,status=%u,"
			"metric=%u,data=%08X,position=%d,bsic=%u,ncc=%u,bcc=%u,frame=%u,com=%04X\n", (uint32_t) i + 1,
			gsm_band_get_name(candidate->band), (uint32_t) candidate->arfcn, result.attempt_count,
			result.refinement_attempt_count, (uint32_t) result.status, (uint32_t) result.metric, result.data,
			(int32_t) result.equalizer_position, (uint32_t) result.bsic, (uint32_t) (result.bsic >> 3),
			(uint32_t) (result.bsic & 7), result.frame_number, (uint32_t) result.communication_flags);
	}

	printf("# SCH_STATS,candidates=%u,decoded_candidates=%u\n", attempted_candidates, decoded_candidates);
	return decoded_candidates != 0;
}

int main(void) {
	static const struct gsm_l1_runtime L1_RUNTIME = {
		.poll = poll_l1_runtime,
		.command_timeout_ms = L1_COMMAND_TIMEOUT_MS,
	};

	test_start("GSM channel scan");
	test_category("Receiver setup");
	stopwatch_t receiver_setup_start = stopwatch_get();
	bool receiver_ready = test_check("Baseband receive path initializes once", gsm_l1_baseband_init(&L1_RUNTIME));

	bool transceiver_ready = false;
	if (receiver_ready)
		transceiver_ready = test_check("RF transceiver initializes through TPU control RAM", gsm_l1_trx_init());

	printf("# STAGE_TIME,stage=receiver_setup,elapsed_ms=%u\n", stopwatch_elapsed_ms(receiver_setup_start));

	test_category("GSM channel scan");
	stopwatch_t channel_scan_start = stopwatch_get();
	bool scan_completed = transceiver_ready && scan_channels();
	test_check("All supported GSM bands scan", scan_completed);
	printf("# STAGE_TIME,stage=channel_scan,elapsed_ms=%u\n", stopwatch_elapsed_ms(channel_scan_start));

	test_category("FCCH and AFC");
	stopwatch_t fcch_scan_start = stopwatch_get();
	bool fcch_completed = scan_completed && search_fcch();
	test_check("FCCH scan completes", fcch_completed);
	printf("# STAGE_TIME,stage=fcch_scan,elapsed_ms=%u\n", stopwatch_elapsed_ms(fcch_scan_start));

	test_category("SCH and BSIC");
	stopwatch_t sch_scan_start = stopwatch_get();
	bool sch_completed = fcch_completed && decode_sch();
	test_check("SCH decodes for at least one FCCH candidate", sch_completed);
	printf("# STAGE_TIME,stage=sch_scan,elapsed_ms=%u\n", stopwatch_elapsed_ms(sch_scan_start));

	gsm_l1_deinit();
	return test_finish();
}

__IRQ void irq_handler(void) {
	uint32_t irq_number = VIC_IRQ_CURRENT;

	if (irq_number == VIC_TPU_INT0_IRQ)
		gsm_l1_tpu_irq();

	VIC_IRQ_ACK = 1;
}
