#include <pmb887x.h>

#include "dmac.h"

void dmac_request_select(uint32_t request, uint32_t select) {
#if DMAC_HAS_REQUEST_SELECTOR
	SCU_DMARS = (SCU_DMARS & ~BIT(request)) | (select << request);
#else
	(void) request;
	(void) select;
#endif
}
