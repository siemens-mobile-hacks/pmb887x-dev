#include <pmb887x.h>

#include "../dsp-container.h"
#include "../dsp-hw.h"
#include "runtime.h"
#include "test.h"

#define DSP_BOOT_DATA_OFFSET 2
#define DSP_RUNTIME_RESULT_OFFSET 0x0200
#define DSP_RUNTIME_SHARED_WORDS 0x0600
#define FNV1A_OFFSET_BASIS 0x811C9DC5
#define FNV1A_PRIME 0x01000193

struct startup_record_manifest {
	uint16_t command;
	uint16_t destination;
	uint16_t words;
	uint32_t payload_hash;
};

#include "startup-manifest.inc"

struct startup_manifest {
	const struct startup_record_manifest *records;
	size_t count;
};

static const struct startup_manifest STARTUP_MANIFESTS[] = {
	{ BASE_STARTUP_MANIFEST, ARRAY_SIZE(BASE_STARTUP_MANIFEST) },
	{ EXTENDED_STARTUP_MANIFEST, ARRAY_SIZE(EXTENDED_STARTUP_MANIFEST) },
};

static uint32_t hash_bytes(const uint8_t *data, size_t size) {
	uint32_t hash = FNV1A_OFFSET_BASIS;

	for (size_t i = 0; i < size; i++)
		hash = (hash ^ data[i]) * FNV1A_PRIME;
	return hash;
}

static uint32_t hash_words(const uint16_t *data, size_t words) {
	uint32_t hash = FNV1A_OFFSET_BASIS;

	for (size_t i = 0; i < words; i++) {
		hash = (hash ^ (uint8_t) data[i]) * FNV1A_PRIME;
		hash = (hash ^ (uint8_t) (data[i] >> 8)) * FNV1A_PRIME;
	}
	return hash;
}

static bool wait_for_boot_record(size_t *polls, uint32_t *elapsed_ticks) {
	uint32_t start = STM_TIM0;

	for (size_t poll = 1; poll <= DSP_RUNTIME_FIRMWARE_POLL_LIMIT; poll++) {
		if ((DSP_COM_STATUS & BIT(0)) == 0) {
			*polls = poll;
			*elapsed_ticks = STM_TIM0 - start;
			return true;
		}
	}

	*polls = DSP_RUNTIME_FIRMWARE_POLL_LIMIT;
	*elapsed_ticks = STM_TIM0 - start;
	return false;
}

static bool submit_record(const dsp_container_record_t *record, size_t *polls, uint32_t *elapsed_ticks) {
	volatile uint16_t *boot_data = dsp_hw_shared_memory + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = record->command;
	boot_data[1] = record->destination;
	if (record->command != DSP_CONTAINER_BRANCH) {
		boot_data[2] = record->words;
		for (size_t i = 0; i < record->words; i++)
			boot_data[i + 3] = dsp_container_read_u16(record->data + i * sizeof(uint16_t));
	}

	DSP_COM_SET = BIT(0);
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;
	return wait_for_boot_record(polls, elapsed_ticks);
}

static bool readback_record(const dsp_container_record_t *record, uint16_t *readback) {
	if (record->command == DSP_CONTAINER_PLOAD)
		return dsp_hw_read_program(record->destination, readback, record->words);
	return dsp_hw_read_data(record->destination, readback, record->words);
}

static bool record_matches_manifest(
	const dsp_container_record_t *record,
	const struct startup_record_manifest *expected
) {
	if (record->command != expected->command)
		return false;
	if (record->destination != expected->destination)
		return false;
	if (record->words != expected->words)
		return false;
	if (record->command == DSP_CONTAINER_BRANCH)
		return expected->payload_hash == 0;

	return hash_bytes(record->data, record->words * sizeof(uint16_t)) == expected->payload_hash;
}

static bool load_and_verify_profile(const struct dsp_runtime_profile *profile, const struct startup_manifest *manifest) {
	uint16_t readback[DSP_HW_BOOT_TRANSFER_WORDS];
	dsp_container_reader_t reader = dsp_container_reader_init(profile->container, profile->container_size);
	dsp_container_record_t record;
	size_t record_index = 0;
	bool loaded = true;

	if (!dsp_hw_reset())
		return false;
	if (dsp_hw_shared_memory[0] != DSP_RUNTIME_MASK_ID)
		return false;
	for (size_t i = 0; i < DSP_RUNTIME_SHARED_WORDS; i++)
		dsp_hw_shared_memory[i] = 0;
	DSP_COM_CLEAR = UINT16_MAX;

	while (dsp_container_next(&reader, &record)) {
		char name[104];

		if (record_index >= manifest->count) {
			test_check("startup stream has no unexpected records", false);
			return false;
		}
		sprintf(name, "startup record %lu matches the independent manifest", (uint32_t) record_index);
		if (!test_check(name, record_matches_manifest(&record, &manifest->records[record_index])))
			return false;

		size_t polls;
		uint32_t elapsed_ticks;
		bool completed = submit_record(&record, &polls, &elapsed_ticks);
		sprintf(name, "startup record %lu completes within firmware's 1000-poll wait",
			(uint32_t) record_index);
		test_check(name, completed);
		printf("# startup record %lu command=%lu address=%04lX words=%lu STM_ticks=%lu polls=%lu\n",
			(uint32_t) record_index, (uint32_t) record.command, (uint32_t) record.destination,
			(uint32_t) record.words, elapsed_ticks, (uint32_t) polls);
		if (!completed)
			return false;

		if (record.command != DSP_CONTAINER_BRANCH) {
			bool read_complete = readback_record(&record, readback);
			sprintf(name, "startup record %lu can be read back completely", (uint32_t) record_index);
			if (!test_check(name, read_complete))
				return false;
			sprintf(name, "startup record %lu hardware contents match its independent checksum",
				(uint32_t) record_index);
			test_eq_u32(name, manifest->records[record_index].payload_hash,
				hash_words(readback, record.words));
		}

		record_index++;
	}

	loaded &= reader.complete;
	loaded &= record_index == manifest->count;
	test_check("startup stream ends after the exact manifest record count", loaded);
	if (!loaded)
		return false;

	dsp_runtime_firmware_delay();
	return test_check("runtime accepts the exact firmware startup probe after the complete stream",
		dsp_runtime_startup_probe(DSP_RUNTIME_RESULT_OFFSET));
}

static void test_profile(size_t profile_index) {
	const struct dsp_runtime_profile *profile = &DSP_RUNTIME_PROFILES[profile_index];
	char category[72];

	sprintf(category, "%s complete startup stream", profile->name);
	test_category(category);
	test_check("complete startup stream satisfies firmware transport contracts",
		load_and_verify_profile(profile, &STARTUP_MANIFESTS[profile_index]));
}

static void test_busy_transport_recovery(void) {
	static const uint16_t PARAMETERS[] = {
		DSP_RUNTIME_ROUTE_PROGRAM_ADDRESS, DSP_RUNTIME_RESULT_OFFSET, 2,
	};

	dsp_runtime_command_prepare(0, 35, PARAMETERS, ARRAY_SIZE(PARAMETERS));
	DSP_COM_SET = BIT(0);
	dsp_runtime_firmware_delay();
	test_eq_u32("runtime pipe remains busy when its interrupt is intentionally omitted",
		BIT(0), DSP_COM_STATUS & BIT(0));
	test_check("reset and complete reload recover a busy runtime transport",
		load_and_verify_profile(&DSP_RUNTIME_PROFILES[0], &STARTUP_MANIFESTS[0]));
}

int main(void) {
	test_start("DSP complete startup transport test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	for (size_t i = 0; i < DSP_RUNTIME_PROFILE_COUNT; i++)
		test_profile(i);

	test_category("Recovery from busy runtime transport");
	test_busy_transport_recovery();

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
