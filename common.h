#ifndef _COMMON_
#define _COMMON_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

#if 0
#define DEBUG
#endif


/* we choose one of the three options */
//#define ATA_RFMQ
//#define W_RFMQ
#define RFMQ


/* fixed value */
#define BUF_SIZE 40

#define R 0
#define W 1

#define DRAM_READ_LATENCY	50
#define DRAM_WRITE_LATENCY	50
#define PCM_READ_LATENCY	100
#define PCM_WRITE_LATENCY	350
#define DRAM_READ_POWER		1
#define DRAM_WRITE_POWER	1
#define PCM_READ_POWER 		2
#define PCM_WRITE_POWER		10
#define STORAGE_LATENCY		5000


/* used in input_ready */
#define INPUT_SIZE	20000
#define ADDRESS_NUM	11000000

/* used in run */
#define DRAM_SIZE	8800000
#define PCM_SIZE	2200000

/* MQ's TH */
#define READ_WEIGHT	3
#define MAX_FRQ		9999999
#define MQ_LEVEL	10

#define VICTIM_DEMOTION	2


/* list macro  */
#define for_each_pcm(num)	for(num=0;num<PCM_SIZE;num++)
#define for_each_dram(num)	for(num=0;num<DRAM_SIZE;num++)
#define for_each_address(num)	for(num=0;num<ADDRESS_NUM;num++)
#define for_each_mq(num)	for(num=0;num<MQ_LEVEL;num++)


// operation conunt stat. R W ratio
struct stat {
	int dram_write_hit;
	int dram_read_hit;
	int pcm_write_hit;
	int pcm_read_hit;
	int write_miss;
	int read_miss;
	int write_back;
	int pcm_to_dram;
	int dram_to_pcm;
	unsigned int latency;
	long power;
};

struct page {
//clock-dwf's
	int num;
	int dirty;
	int ref;
	int frq;
	int over;
//mq's
	int pre_level; //added my_mq
	int level; // added my_mq
	int read_count;	//added my_mq
	int demote_count;
	int victim_count;
	int isDRAM;
	int frame_number;
	struct list_head mq;
	struct list_head victim;
	struct list_head wait;

//4Q's
	struct list_head position;
};

//mq's
struct mqvec {
	struct list_head lists[MQ_LEVEL];
};

//4Q's
struct kaist_list {
	struct list_head dram_read;
	struct list_head dram_write;
	struct list_head pcm_write;
	struct list_head pcm_read;
};



//common

struct free_index{
	int frame_num;
	struct list_head free_list;
};

struct dram_frame {
	int hand; //clock-dwf's clock hand
	struct page *page[DRAM_SIZE];
	struct free_index index[DRAM_SIZE];
	struct list_head free_list;
};

struct pcm_frame {
	int hand;
	struct page *page[PCM_SIZE];
	struct free_index index[PCM_SIZE];
	struct list_head free_list;
};

struct input {
	int num;
	int type;
	long address;
};

struct access_information {
	int num;
	long address;
	int overlab_count;
	int w_count;
	int r_count;
};


#endif
