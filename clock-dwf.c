#include "clock-dwf.h"


void init_pcm_page(struct page *p)
{
	p->dirty = 0;
	p->frq = 0;
	p->over = 0;
	p->ref = 0;
	p->isDRAM = -1;
	p->frame_number = 0;
}
void writeback_PCM(struct page *p, struct stat *stat)
{
#ifdef DEBUG
	printf("PCM %d  Write_back!!!\n", p->num);
#endif
	init_pcm_page(p);
	stat->write_back++;
}

void PCM_flush(struct pcm_frame *PCM, struct page *p)
{
	PCM->page[p->frame_number] = NULL;
	list_move_tail(&PCM->index[p->frame_number].free_list, &PCM->free_list);
	init_pcm_page(p);
}

void set_dram(struct dram_frame *DRAM, struct page *input, int to_frame_num)
{
	DRAM->page[to_frame_num] = input;
	input->frame_number = to_frame_num;
	input->isDRAM = 1;
}

void set_pcm(struct pcm_frame *PCM, struct page *input, int to_frame_num)
{
	PCM->page[to_frame_num] = input;
	input->frame_number = to_frame_num;
	input->isDRAM = 0;
}


int get_free_page_of_pcm(struct pcm_frame *PCM, struct stat *stat)
{
	int s;
	struct page *p;

	s = is_free_pcm(PCM);
	if (s == -1) {
		while (1) {
			p = PCM->page[PCM->hand];
			s = PCM->hand;
			PCM->hand++;
			if (PCM->hand >= PCM_SIZE)
				PCM->hand = 0;

			if (p->ref == 1){
#ifdef DEBUG
				printf("page %d's ref set to '0'\n", p->num);
#endif
				p->ref = 0;
				continue;
			} else {
#ifdef DEBUG
				printf("get_free_page_of_pcm() = %d\n", s);
#endif
				if (p->dirty == 1){
					writeback_PCM(p, stat);
				}
				return s;
			}
		}
	} else {
#ifdef DEBUG
		printf("PCM has free frame -> get_free_page_of_pcm() = %d\n", s);
#endif
		return s;
	}
}

int get_free_page_of_dram(struct dram_frame *DRAM, struct pcm_frame *PCM,
			  struct stat *stat, int hot_page_th, int expiration)
{
	int s, hand;
	struct page *p;

	s = is_free_dram(DRAM);
	if (s == -1) {
		while(1) {
			p = DRAM->page[DRAM->hand];
			hand = DRAM->hand;
			DRAM->hand++;
			if (DRAM->hand >= DRAM_SIZE)
				DRAM->hand = 0;
			if (p->dirty == 0) {
				if (p->frq > hot_page_th &&
				    p->over < expiration) {
#ifdef DEBUG
					printf("%d page's frq = %d, over = %d\n",
					       p->num, p->frq, p->over);
#endif
					(p->over)++;
				} else {
					s = get_free_page_of_pcm(PCM, stat);
					set_pcm(PCM, p, s);
					p->dirty = 1;
					stat->dram_to_pcm++;
#ifdef DEBUG
					printf("DRAM -> PCM Migration!! \n");
					printf("get_free_page_of_dram() = %d\n",
					       hand);
#endif
					return hand;
				}
			} else {
				p->dirty = 0;
				p->frq++;
				p->over	= 0;
			}
		}
	} else {
#ifdef DEBUG
		printf("get_free_page_of_dram() = %d\n", s);
#endif
		return s;
	}
}


void clock_dwf(struct dram_frame *DRAM, struct pcm_frame *PCM,
	       struct page *p, int op, struct stat *stat,
	       int hot_page_th, int expiration)
{
	int s;

	if (p->isDRAM == 1) {
		if (op == R) {
#ifdef DEBUG
			printf("Read %d : DRAM HIT\n", p->num);
#endif
			stat->dram_read_hit++;
			p->ref = 1;
		} else {
#ifdef DEBUG
			printf("Write %d : DRAM HIT\n", p->num);
#endif
			stat->dram_write_hit++;
			p->dirty = 1;
			p->ref = 1;
		}
	} else if (p->isDRAM == 0) {
		if (op == R) {
#ifdef DEBUG
			printf("Read %d : PCM HIT\n", p->num);
#endif
			stat->pcm_read_hit++;
			p->ref = 1;
		} else {
#ifdef DEBUG
			printf("Write %d : PCM HIT\n", p->num);
			printf("PCM -> DRAM Migration!!\n");
#endif
			stat->pcm_write_hit++;
			stat->pcm_to_dram++;
			s = get_free_page_of_dram(DRAM, PCM, stat, hot_page_th,
						  expiration);
			PCM_flush(PCM, p);
			set_dram(DRAM, p, s);
		}
	} else {
		if (op == R) {
#ifdef DEBUG
			printf("Read %d : MISS!!\n", p->num);
#endif
			stat->read_miss++;
			s = get_free_page_of_pcm(PCM, stat);
			set_pcm(PCM, p, s);
			p->ref = 0;
		} else {
#ifdef DEBUG
			printf("Write %d : MISS!!\n", p->num);
#endif
			stat->write_miss++;
			s = get_free_page_of_dram(DRAM, PCM, stat, hot_page_th,
						  expiration);
			set_dram(DRAM, p, s);
			p->dirty = 0;
		}
	}
}

void simulation_clock_dwf(char *input_file, struct dram_frame *DRAM,
			  struct pcm_frame *PCM, struct page *page,
			  struct stat *stat, int hot_page_th, int expiration)
{
	int i=0;
	FILE *f = fopen(input_file, "r");
	int check=0;
	char buf[BUF_SIZE];
	struct input input;

	if (f != NULL) {
		while(fgets(buf, sizeof(buf), f) != NULL) {
			i++;
			init_input(&input);
			if (token_split(&input, buf)){
#ifdef DEBUG
				printf("\n===============================================\n");
				printf("%dth DRAM's clock hand = %d, PCM's clock hand = %d\n",
				       i+1,DRAM->hand, PCM->hand);
				printf("============= %c, %d ==========\n",
				       input.type == 0 ? 'R' : 'W', input.num);
#endif
				clock_dwf(DRAM, PCM, &page[input.num],
					  input.type, stat, hot_page_th,
					  expiration);
#ifdef DEBUG
				print_RAMS(DRAM, PCM);
#endif
			} else {
				printf("Tokenizing fail!!\n");
			}
		}
	} else {
		printf("File empty\n");
	}

	fclose(f);

#ifdef DEBUG
	printf("\n#################################################\n");
	printf("###############   E   N   D  ####################\n");
	printf("#################################################\n\n");
	printf("hot_page_th : %d, expiration : %d\n", hot_page_th, expiration);
#endif
}
