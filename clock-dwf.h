#ifndef _CLOCK_DWF_
#define _CLOCK_DWF_

#include "common.h"
#include "main.h"

void simulation_clock_dwf(char *input_file, struct dram_frame *DRAM,
			  struct pcm_frame *PCM, struct page *page, struct stat *stat,
			  int hot_page_th, int expiration);
#endif
