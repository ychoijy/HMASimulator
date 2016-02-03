#include "main.h"

struct dram_frame DRAM;
struct pcm_frame PCM;


/*
 *	Shared method (clock-dwf, mq, my_mq, etc..)
 */
void init_all(struct dram_frame *DRAM, struct pcm_frame *PCM, struct page *page,
	      struct stat *stat, struct mqvec *mqvec,
	      struct mqvec *mqvec_dram, struct list_head *victim_list,
	      struct list_head *wait_list, int demote_period)
{
	int i;
	DRAM->hand = 0;
	PCM->hand = 0;
	INIT_LIST_HEAD(&DRAM->free_list);
	INIT_LIST_HEAD(&PCM->free_list);

	for_each_dram(i) {
		DRAM->page[i] = NULL;
		DRAM->index[i].frame_num = i;
		INIT_LIST_HEAD(&DRAM->index[i].free_list);
		list_move_tail(&DRAM->index[i].free_list, &DRAM->free_list);
	}

	for_each_pcm(i) {
		PCM->page[i] = NULL;
		PCM->index[i].frame_num = i;
		INIT_LIST_HEAD(&PCM->index[i].free_list);
		list_move_tail(&PCM->index[i].free_list, &PCM->free_list);
	}


	for_each_address(i){
		page[i].num = i;
		page[i].dirty = 0;
		page[i].ref = 0;
		page[i].frq = 0;
		page[i].over = 0;
		page[i].level = 0;
		page[i].pre_level = 0;
		page[i].read_count = 0;
		page[i].frame_number = 0;
		page[i].isDRAM = -1;
// MQ's
		page[i].demote_count = demote_period;
		page[i].victim_count = 0;
		INIT_LIST_HEAD(&page[i].mq);
		INIT_LIST_HEAD(&page[i].wait);
		INIT_LIST_HEAD(&page[i].victim);
	}

	for_each_mq(i) {
		INIT_LIST_HEAD(&mqvec->lists[i]);
		INIT_LIST_HEAD(&mqvec_dram->lists[i]);
	}
	INIT_LIST_HEAD(victim_list);
	INIT_LIST_HEAD(wait_list);
//
	stat->dram_write_hit = 0;
	stat->dram_read_hit = 0;
	stat->pcm_write_hit = 0;
	stat->pcm_read_hit = 0;
	stat->write_miss = 0;
	stat->read_miss = 0;
	stat->write_back = 0;
	stat->pcm_to_dram = 0;
	stat->dram_to_pcm = 0;
	stat->latency = 0;
	stat->power = 0.0;
}

int is_free_pcm(struct pcm_frame *PCM)
{
//	int i;
	struct free_index *index;

	if (!list_empty(&PCM->free_list)) {
		list_for_each_entry(index, &PCM->free_list, free_list) {
			if (index != NULL) {
				int frame_num = index->frame_num;
				list_del_init(&index->free_list);
				return frame_num;
			}
		}

	}
/*
	for_each_pcm(i) {
		if (PCM->page[i] == NULL)
			return i;
	}
	*/
	return -1;
}

int is_free_dram(struct dram_frame *DRAM)
{
//	int i;
	struct free_index *index;

	if (!list_empty(&DRAM->free_list)) {
		list_for_each_entry(index, &DRAM->free_list, free_list) {
			if (index != NULL) {
				int frame_num = index->frame_num;
				list_del_init(&index->free_list);
				return frame_num;
			}
		}

	}
/*
	for_each_dram(i) {
		if (DRAM->page[i] == NULL)
			return i;
	}
	*/
	return -1;
}

/*
 * print state
 */
void print_stat(struct stat *stat)
{
	printf("===== D:%d P:%d statistic =====\n", DRAM_SIZE, PCM_SIZE);
	printf("dram_write_hit = %d, dram_read_hit = %d\n", stat->dram_write_hit, stat->dram_read_hit);
	printf("pcm_write_hit = %d, pcm_read_hit = %d\n", stat->pcm_write_hit, stat->pcm_read_hit);
	printf("write_miss = %d, read_miss = %d\n", stat->write_miss, stat->read_miss);
	printf("pcm_to_dram = %d, dram_to_pcm = %d, write_back = %d\n", stat->pcm_to_dram, stat->dram_to_pcm, stat->write_back);
	printf("latency = %d, power = %ld\n", stat->latency, stat->power);
	printf("==================================\n");

}

void print_RAMS(struct dram_frame *DRAM, struct pcm_frame *PCM)
{
	int i;

	printf("==============================\n");
	printf("#######  DRAM  ####### | #######  PCM  #######\n");

	if (DRAM_SIZE > PCM_SIZE){
		for (i=0;i<PCM_SIZE;i++){
			printf("%6d (%2d)(%2d)\t\t%6d (%2d)(%2d)\n",
			       DRAM->page[i] == NULL ? -1 : DRAM->page[i]->num,
			       DRAM->page[i] == NULL ? -1 : DRAM->page[i]->frq,
			       DRAM->page[i] == NULL ? -1 : DRAM->page[i]->read_count,
			       PCM->page[i] == NULL ? -1 : PCM->page[i]->num,
			       PCM->page[i] == NULL ? -1 : PCM->page[i]->frq,
			       PCM->page[i] == NULL ? -1 : PCM->page[i]->read_count);
		}
		for (;i<DRAM_SIZE;i++){
			printf("%6d (%2d)(%2d)\n",
			       DRAM->page[i] == NULL ? -1 : DRAM->page[i]->num,
			       DRAM->page[i] == NULL ? -1 : DRAM->page[i]->frq,
			       DRAM->page[i] == NULL ? -1 : DRAM->page[i]->read_count);
		}
	} else {
		for (i=0;i<DRAM_SIZE;i++){
			printf("%6d (%2d)(%2d)\t\t%6d (%2d)(%2d)\n",
			       DRAM->page[i] == NULL ? -1 : DRAM->page[i]->num,
			       DRAM->page[i] == NULL ? -1 : DRAM->page[i]->frq,
			       DRAM->page[i] == NULL ? -1 : DRAM->page[i]->read_count,
			       PCM->page[i] == NULL ? -1 : PCM->page[i]->num,
			       PCM->page[i] == NULL ? -1 : PCM->page[i]->frq,
			       PCM->page[i] == NULL ? -1 : PCM->page[i]->read_count);
		}
		for (;i<PCM_SIZE;i++){
			printf("\t\t\t%6d (%2d)(%2d)\n",
			       DRAM->page[i] == NULL ? -1 : DRAM->page[i]->num,
			       DRAM->page[i] == NULL ? -1 : DRAM->page[i]->frq,
			       DRAM->page[i] == NULL ? -1 : DRAM->page[i]->read_count);
		}

	}
}

void print_menu()
{
	printf("-----------------------------\n");
	printf("Select Algorithm\n");
	printf(" 1. Clock-DWF\n");
	printf(" 2. MQ\n");
	printf(" 3. My_MQ\n");
	printf("-----------------------------\n");
	printf("-->> ");
}

int token_split(struct input *input, char *buf)
{
	char *ptr, *stop;
	int i=0;

	ptr = strtok(buf, " ");


	while ((ptr = strtok(NULL, " ")) != NULL) {
		if (i == 0) {
			if (atoi(ptr) != 0) {
				input->type = W;
				input->num = atoi(ptr);
				return 1;
			}
			i++;
		} else {
			input->type = R;
			input->num = atoi(ptr);
			return 1;
		}
	}
	return 1;
}

void calc_stat(struct stat *stat)
{
	stat->latency = (stat->dram_write_hit * DRAM_WRITE_LATENCY) +
		(stat->dram_read_hit * DRAM_READ_LATENCY) +
		(stat->pcm_write_hit * PCM_WRITE_LATENCY) +
		(stat->pcm_read_hit * PCM_READ_LATENCY) +
		(stat->pcm_to_dram * DRAM_WRITE_LATENCY) +
		(stat->dram_to_pcm * PCM_WRITE_LATENCY) +
		(stat->write_miss * STORAGE_LATENCY) +
		(stat->read_miss * STORAGE_LATENCY);

/*
   by clock-dwf article
Ptotal = Pstatic + Pactive
Pstatic = unit_static_power (W/GB) * memory_size (GB) and
Pactive = ( Nread * Eread(J) + Nwrite * Ewrite(J)) / ( Nread * Lread(ns) + Nwrite * Lwrite(ns)).
*/
	stat->power = ((stat->dram_write_hit * DRAM_WRITE_POWER) +
		(stat->dram_read_hit * DRAM_READ_POWER) +
		(stat->pcm_write_hit * PCM_WRITE_POWER) +
		(stat->pcm_read_hit * PCM_READ_POWER) +
		(stat->pcm_to_dram * DRAM_WRITE_POWER) +
		(stat->dram_to_pcm * PCM_WRITE_POWER)) / 10;
}

void init_input(struct input *input)
{
	input->num = -100;
	input->type = -100;
	input->address = -100;
}

int main(int argc, char *argv[])
{
	int hot_page_th, expiration;
	int mq_migrate_th, demote_period;
	struct page *page;
	struct stat stat;

	// MQ only use
	struct mqvec mqvec;
	struct mqvec mqvec_dram; //my_mq is two mq...

	struct list_head victim_list;
	struct list_head wait_list; // added my_mq
	//

	FILE *result_file;
	if (argc != 5) {
		printf("Useage : ./run tracefile algorithm 1 1\n");
		printf("algorithm -> 1 = CLOCK-DWF, 2 = MQ\n");
		printf("./run tracefile 1 [hot_page_th] [expiration]\n");
		printf("./run tracefile 2 [mq_migrate_th] [demote_period]\n");
		return 1;
	}

	if (atoi(argv[2]) == 1) {
		hot_page_th = atoi(argv[3]);
		expiration = atoi(argv[4]);
	} else if (atoi(argv[2]) == 2) {
		mq_migrate_th = atoi(argv[3]);
		demote_period = atoi(argv[4]);
	}

	page = (struct page *)malloc(sizeof(struct page) * ADDRESS_NUM);

	result_file = fopen("./stat.dat", "a");

	init_all(&DRAM, &PCM, page, &stat, &mqvec, &mqvec_dram,
		 &victim_list, &wait_list, demote_period);

	switch (atoi(argv[2])) {
		case 1:
			simulation_clock_dwf(argv[1], &DRAM, &PCM, page,
				     	&stat, hot_page_th, expiration);
			break;
		case 2:
			simulation_my_mq(argv[1], &DRAM, &PCM, page,
					 &stat, &mqvec, &mqvec_dram,
					 &victim_list, &wait_list,
					 mq_migrate_th, demote_period);
			break;
		default:
			break;
	}
	//print_RAMS(&DRAM, &PCM);
	calc_stat(&stat);
	print_stat(&stat);

	fprintf(result_file, "%d\t%d\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%ld\n",
		atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
		stat.dram_write_hit, stat.dram_read_hit, stat.pcm_write_hit,
		stat.pcm_read_hit, stat.write_miss, stat.read_miss,
		stat.pcm_to_dram, stat.dram_to_pcm, stat.write_back,
		stat.latency, stat.power);

	free(page);
	fclose(result_file);

	return 0;
}
