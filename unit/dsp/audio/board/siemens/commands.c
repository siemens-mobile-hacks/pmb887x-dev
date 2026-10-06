#include <pmb887x.h>

#include "dsp/audio/board/siemens/commands.h"
#include "dsp/runtime.h"

bool dsp_siemens_cmd_exec(
	size_t pipe_index,
	uint16_t command,
	const uint16_t *parameters,
	size_t parameter_count
) {
	dsp_runtime_command_start(pipe_index, command, parameters, parameter_count);
	if (!dsp_runtime_wait(BIT(pipe_index))) {
		printf("# Siemens audio command timed out: command=%u pipe=%lu\n",
			command, (uint32_t) pipe_index);
		return false;
	}

	// DConfJTab does not preserve the dispatcher's acknowledgement value.
	if (command == SIEMENS_AUDIO_COMMAND_DCONF_JTAB)
		return true;
	if (dsp_runtime_pipe_get(pipe_index)[0] == (uint16_t) -command)
		return true;

	printf("# Siemens audio command rejected: command=%u pipe=%lu response=%04X\n",
		command, (uint32_t) pipe_index, dsp_runtime_pipe_get(pipe_index)[0]);
	return false;
}

bool dsp_siemens_jtab_exec(
	size_t pipe_index,
	const uint16_t *words,
	size_t word_count
) {
	for (size_t offset = 0; offset < word_count;) {
		size_t record_word_count = (size_t) words[offset] + 1;
		if (record_word_count > word_count - offset) {
			printf("# Truncated DConfJTab record: offset=%lu words=%lu remaining=%lu\n",
				(uint32_t) offset, (uint32_t) record_word_count, (uint32_t) (word_count - offset));
			return false;
		}
		if (!dsp_siemens_cmd_exec(pipe_index, SIEMENS_AUDIO_COMMAND_DCONF_JTAB,
			&words[offset], record_word_count)) {
			return false;
		}
		offset += record_word_count;
	}
	return true;
}
