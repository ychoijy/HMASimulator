#ifndef _My_MQ_
#define _My_MQ_

#include "common.h"
#include "main.h"
void simulation_my_mq(char *input_file, struct dram_frame *DRAM,
			  struct pcm_frame *PCM, struct page *page, struct stat *stat,
			  struct mqvec *mqvec, struct mqvec *mqvec_dram,
			  struct list_head *victim_list, struct list_head *wait_list,
			  int mq_migrate_th, int demote_period);
#endif

