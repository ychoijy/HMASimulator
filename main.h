#ifndef _MAIN_
#define _MAIN_

#include <string.h>
#include "common.h"
#include "clock-dwf.h"
#include "my_mq.h"

int is_free_pcm(struct pcm_frame *PCM);
int is_free_dram(struct dram_frame *DRAM);
int check_dram(struct dram_frame *DRAM, struct page *p);
int check_pcm(struct pcm_frame *PCM, struct page *p);


void print_stat(struct stat *stat);
void print_inputs(struct input *input);
void print_RAMS(struct dram_frame *DRAM, struct pcm_frame *PCM);

#endif
