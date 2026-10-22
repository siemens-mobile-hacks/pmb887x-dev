#pragma once

#include <stdbool.h>
#include <stdint.h>

struct dif_lane_config {
	const char *name;
	uint32_t con;
	uint32_t txfifo_cfg;
	uint32_t bsconf;
};

enum dif_blit_source {
	DIF_BLIT_EXTRAM,
	DIF_BLIT_SRAM,
};

struct dif_bus_timing {
	bool ok;
	uint32_t long_ns;
	uint32_t short_ns;
	/* Steady-state parallel bus time per byte, in 1/100 ns. */
	uint32_t byte_ns100;
	/* State captured when a blit did not complete. */
	uint32_t failed_words;
	uint32_t failed_dma_remaining;
	uint32_t failed_stat;
	uint32_t failed_txffs;
	uint32_t failed_errirqss;
};

extern const struct dif_lane_config DIF_LANE_1x8;
extern const struct dif_lane_config DIF_LANE_2x8;
extern const struct dif_lane_config DIF_LANE_4x8;

/* Returns false when the SRAM source buffer does not fit after the linked .sram section. */
bool dif_bus_timing_init(void);
uint32_t dif_lcdtim1(uint32_t addr_delay, uint32_t access_cycle, uint32_t data_delay);
uint32_t dif_lcdtim2(uint32_t cs_act, uint32_t cs_deact, uint32_t wr_act, uint32_t wr_deact);
/* Does not print, so callers may measure while the console clock is changed. */
struct dif_bus_timing dif_bus_timing_measure(const struct dif_lane_config *lane, enum dif_blit_source source,
	uint32_t clc, uint32_t tim1, uint32_t tim2);
void dif_bus_timing_print_failure(const struct dif_bus_timing *timing);
