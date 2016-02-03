#include "my_mq.h"

/*
 *  Below, My_Multi queue algorithm
 */
void print_my_mq(struct mqvec *mqvec)
{
	int i, j;
	struct page *p;

	printf("=======MQ LEVEL======\n");
	for_each_mq(i) {
		printf("level %d : ", i);
		j=1;
		list_for_each_entry(p, &mqvec->lists[i], mq){
			if( p != NULL )
				printf("%6d%s", p->num, (j % 10) != 0 ? "," : "\n\t  ");
			j++;
		}
		printf("\n");
	}
	printf("=====================\n");
}

void print_wait_list(struct list_head *wait_list)
{
	struct page *p;
	int i=1;

	printf("\n======== wait list =======\n");
	list_for_each_entry(p, wait_list, wait) {
		if (p != NULL)
			printf("%6d%s", p->num, (i % 10) != 0 ? "," : "\n");
		i++;
	}
	printf("\n=====================\n");
}

void print_victim_list(struct list_head *victim_list)
{
	struct page *p;
	int i=1;

	printf("\n===== victim list ====\n");
	list_for_each_entry(p, victim_list, victim) {
		if (p != NULL)
			printf("%6d%s", p->num, (i % 10) != 0 ? "," : "\n");
		i++;
	}
	printf("\n=====================\n");
}

/*
 *	PCM -> Storage....
 */
void page_init_all(struct page *page)
{

	page->demote_count = 0;
	page->victim_count = 0;
	page->dirty = 0;
	page->frq = 0;
	page->level = 0;
	page->pre_level = 0;
	page->read_count = 0;
	page->frame_number = 0;
	page->isDRAM = -1;

	if (!list_empty(&page->mq)) {
		list_del_init(&page->mq);
	}
	if (!list_empty(&page->wait)) {
		list_del_init(&page->wait);
	}
	if (!list_empty(&page->victim)) {
		list_del_init(&page->victim);
	}

	page->over = 0;
	page->ref = 0;
}

void dram_page_init(struct page *page)
{
	page->read_count = 0;
	page->demote_count = 0;
	page->victim_count = 0;
	page->frq = 0;

	if (page->pre_level == 0 ) {
		page->pre_level = page->level;
	} else {
		page->pre_level = (page->pre_level + page->level) / 2;
	}
	page->level = 0;

	if (!list_empty(&page->mq)) {
		list_del_init(&page->mq);
	}

	if (!list_empty(&page->victim)) {
		list_del_init(&page->victim);
	}
}

/*
 * 	- Storage -> PCM
 *		- victim select
 *			1. Wait_queue -> Storage
 *			2. PCM -> Storage
 * 	considering -> (Frame, list)
 */
void set_to_dram(struct dram_frame *DRAM, struct page *input, int to_frame_num,
	 struct mqvec *mqvec)
{
	DRAM->page[to_frame_num] = input;
	input->frame_number = to_frame_num;
	input->isDRAM = 1;

	list_move_tail(&input->mq, &mqvec->lists[0]);
}

void set_to_pcm(struct pcm_frame *PCM, struct page *input, int to_frame_num,
	 struct mqvec *mqvec)
{
	PCM->page[to_frame_num] = input;
	input->frame_number = to_frame_num;
	input->isDRAM = 0;

	list_move_tail(&input->mq, &mqvec->lists[0]);
}

int write_back(struct pcm_frame *PCM, struct page *victim,
	       struct stat *stat)
{
	int frame_num;

	if(victim->dirty == 1) {
		stat->write_back++;
#ifdef DEBUG
		printf("WriteBack!! :: %d\n", victim->num);
#endif
	} else {
#ifdef DEBUG
		printf("No WriteBack!! :: %d\n", victim->num);
#endif
	}
	frame_num = victim->frame_number;
	page_init_all(PCM->page[victim->frame_number]);

	return frame_num;
}

/*
 *	(PCM<->DRAM)  page exchange. if DRAM is full, choose the victim of dram.
 *	And it is moved to wait_list of PCM.
 */
void exchange(struct dram_frame *DRAM, struct pcm_frame *PCM,
	      struct page *pcm, struct page *dram,
	      struct list_head *wait_list, struct stat *stat)
{
	int temp;
#ifdef DEBUG
	printf("exchange DRAM : %d <-> PCM : %d\n", dram->num, pcm->num);
#endif
	stat->dram_to_pcm++;
	stat->pcm_to_dram++;

	dram_page_init(dram);

	list_move_tail(&dram->wait, wait_list);

	// exchange PCM, DRAM frame.
	DRAM->page[dram->frame_number] = pcm;
	PCM->page[pcm->frame_number] = dram;
	temp = dram->frame_number;
	dram->frame_number = pcm->frame_number;
	dram->isDRAM = 0;
	pcm->frame_number = temp;
	pcm->isDRAM = 1;

}

struct page *select_victim(struct list_head *list,
			   struct mqvec *mqvec,int isDRAM)
{
	int i;
	struct page *page;

	if(!list_empty(list)){
		if (isDRAM) {
			list_for_each_entry(page, list, victim) {
				if (page != NULL)
					return page;
			}
		} else {
			list_for_each_entry(page, list, wait) {
				if (page != NULL)
					return page;
			}
		}
	}

	for_each_mq(i) {
		if (!list_empty(&mqvec->lists[i])) {
			list_for_each_entry(page, &mqvec->lists[i], mq) {
				if ( page != NULL)
					return page;
			}
		}
	}
}

int migration_to_pcm(struct pcm_frame *PCM, struct page *dram,
		     int pcm_frame_num, struct list_head *wait_list,
		     struct stat *stat)
{
	int dram_frame_num = dram->frame_number;

#ifdef DEBUG
	printf(" DRAM->PCM migration!!\n");
#endif
	stat->dram_to_pcm++;

	dram_page_init(dram);

	list_move_tail(&dram->wait, wait_list);

	PCM->page[pcm_frame_num] = dram;
	dram->frame_number = pcm_frame_num;
	dram->isDRAM = 0;

	return dram_frame_num;
}
int migration_to_dram(struct dram_frame *DRAM, struct pcm_frame *PCM, struct page *p,
		       struct list_head *victim_list, struct list_head *wait_list,
		       struct stat *stat, struct mqvec *mqvec_dram, int to_level)
{
	int s, i, j;
	struct page *dram;
	int isDRAM = 1;

	s = is_free_dram(DRAM);

	/*
	 *  DRAM is FULL. So select DRAM's victim page.
	 */
	if (s == -1) {
		/*
		 *  First, we select a victim page of DRAM at victim list
		 *	and exchange it.
		 */
		dram = select_victim(victim_list, mqvec_dram, isDRAM);
		exchange(DRAM, PCM, p, dram, wait_list, stat);
	} else {
		// DRAM space is not FULL, just migration PCM->DRAM
		PCM->page[p->frame_number] = NULL;
		list_move_tail(&PCM->index[p->frame_number].free_list, &PCM->free_list);
		DRAM->page[s] = p;
		p->frame_number = s;
		p->isDRAM = 1;
#ifdef DEBUG
		printf("PCM -> DRAM migration!!\n");
#endif
		stat->pcm_to_dram++;
	}

	list_move_tail(&p->mq, &mqvec_dram->lists[to_level]);
	return 0;
}

void check_promote(struct dram_frame *DRAM, struct pcm_frame *PCM, int isDRAM,
		   struct page *p, struct mqvec *mqvec_pcm, struct mqvec *mqvec_dram,
		   struct list_head *victim_list, struct list_head *wait_list,
		   struct stat *stat, int mq_migrate_th)
{
	int i, j, s;
	int pre = p->frq - 1;
	int cur = p->frq;

	/*
	 *	The pre 0 CASE
	 *  		1. dram's level 0 because of demotion -> pass
	 *  		2. pcm's level 0 because of demotion -> pass
	 *  		3. wait_list
	 */
	if (pre == 0) {
		if (!list_empty(&p->wait)) {	// wait_list -> DRAM or PCM
			list_del_init(&p->wait);

			if (p->pre_level >= mq_migrate_th) {	//  wait_list -> DRAM
#ifdef DEBUG
				printf("[%s] page %d: Wait_list -> DRAM\n", __func__, p->num);
				printf("p->prelevel = %d, pre_determined = %d\n", p->pre_level, p->num);
#endif
				stat->pcm_to_dram++;

				migration_to_dram(DRAM, PCM, p, victim_list,
						  wait_list, stat, mqvec_dram, 0);

			} else {	// wait_list -> PCM
#ifdef DEBUG
				printf("[%s] page %d: Wait_list -> PCM\n", __func__, p->num);
#endif
				list_move_tail(&p->mq, &mqvec_pcm->lists[0]);
			}
		}
	} else {
		for (i=0;pre != 0; i++)
			pre = pre >> 1;

		for (j=0;cur != 0; j++)
			cur = cur >> 1;

		if (i != j) {
			if (j-1 < MQ_LEVEL) {
#ifdef DEBUG
				printf("[%s] %d Move level: %d -> %d\n", __func__, p->num,j-2,j-1);
#endif
				p->level = j-1;
				if (!isDRAM) {
					if ((p->level) == mq_migrate_th) {
						migration_to_dram(DRAM, PCM, p,
							victim_list, wait_list,
							stat, mqvec_dram, p->level);
					} else {
						list_move_tail(&p->mq, &mqvec_pcm->lists[p->level]);
					}
				} else {
					list_move_tail(&p->mq, &mqvec_dram->lists[p->level]);
				}
			}
		}
	}
}

/*
 *	1. Wait_queue -> PCM
 *	2. Wait_queue -> DRAM
 *	3. PCM -> PCM
 *	4. PCM -> DRAM
 *	5. DRAM -> DRAM
 *	6. DRAM -> Wait_queue
 */
void handle_op(struct dram_frame *DRAM, struct pcm_frame *PCM, int isDRAM,
	       struct page *p, struct mqvec *mqvec_pcm, struct mqvec *mqvec_dram,
	       struct list_head *victim_list, struct list_head *wait_list,  struct stat *stat,
	       int mq_migrate_th)
{
	p->frq++;
	p->demote_count = 0;
	p->victim_count = 0;

	/* If page is in the victim list, Reset it.
	 */
	if (!list_empty(&p->victim)) {
#ifdef DEBUG
		printf("Delete victim list : %d\n", p->num);
#endif
		list_del_init(&p->victim);
	}

	check_promote(DRAM, PCM, isDRAM, p, mqvec_pcm, mqvec_dram,
			      victim_list, wait_list, stat, mq_migrate_th);
}

void handle_write_op(struct dram_frame *DRAM, struct pcm_frame *PCM, int isDRAM,
		    struct page *p, struct mqvec *mqvec_pcm, struct mqvec *mqvec_dram,
		    struct list_head *victim_list,struct list_head *wait_list,
		    struct stat *stat, int mq_migrate_th)
{
	p->dirty = 1;
	p->ref = 1;
	handle_op(DRAM, PCM, isDRAM, p, mqvec_pcm, mqvec_dram,
		  victim_list, wait_list, stat, mq_migrate_th);
}

void handle_read_op(struct dram_frame *DRAM, struct pcm_frame *PCM, int isDRAM,
		    struct page *p, struct mqvec *mqvec_pcm, struct mqvec *mqvec_dram,
		    struct list_head *victim_list, struct list_head *wait_list,
		    struct stat *stat, int mq_migrate_th)
{
	p->ref = 1;
#ifdef ATA_RFMQ
	if (p->frq != 0) {
		if ( p->read_count < READ_WEIGHT ) {
			p->read_count++;
		} else {
			p->read_count = 0;
			handle_op(DRAM, PCM, isDRAM, p, mqvec_pcm, mqvec_dram,
				  victim_list, wait_list, stat, mq_migrate_th);
		}
	} else  {
		handle_op(DRAM, PCM, isDRAM, p, mqvec_pcm, mqvec_dram,
			  victim_list, wait_list, stat, mq_migrate_th);
	}
#endif

#ifdef W_RFMQ
	if (p->frq == 0) {
		handle_op(DRAM, PCM, isDRAM, p, mqvec_pcm, mqvec_dram,
			  victim_list, wait_list, stat, mq_migrate_th);
	}
#endif

#ifdef RFMQ
	handle_op(DRAM, PCM, isDRAM, p, mqvec_pcm, mqvec_dram,
		  victim_list, wait_list, stat, mq_migrate_th);
#endif
}

void check_demote(struct dram_frame *DRAM, struct pcm_frame *PCM, int isDRAM,
		  struct page *p, struct list_head *victim_list,
		  struct mqvec *target_mq, struct list_head *wait_list)
{
	int i, j, s;
	int pre = p->frq + 1;
	int cur = p->frq;

	if (!list_empty(&p->mq)) {
		if (cur == 0) {
			/*
			 * current frequency is 0.. so this page is the target of victim or writeback
			 */
			if (isDRAM) {
				if (list_empty(&p->victim)){
#ifdef DEBUG
					printf("[%s] %d frq is 0. so insert to the victim queue\n",
					       __func__, p->num);
#endif
					list_move_tail(&p->victim, victim_list);
				}
			} else {
#ifdef DEBUG
				printf("[%s] %d frq is 0. so insert to the wait queue\n",
				       __func__, p->num);
#endif
				list_del_init(&p->mq);
				list_move_tail(&p->wait, wait_list);
			}
		} else {
			for (i=0;pre != 0; i++)
				pre = pre >> 1;

			for (j=0;cur != 0; j++)
				cur = cur >> 1;

			if (i != j) {
				p->level = j-1;
#ifdef DEBUG
				printf("[%s] page %d, level : %d -> %d\n",__func__, p->num, j, j-1);
#endif
				list_move_tail(&p->mq, &target_mq->lists[p->level]);

				p->victim_count++;
				if (isDRAM) {
					if (list_empty(&p->victim) && p->victim_count >= VICTIM_DEMOTION) {
#ifdef DEBUG
						printf("[%s] %d added victim_list\n",
						       __func__, p->num);
#endif
						list_move_tail(&p->victim, victim_list);
					}
				} else {
					if (p->victim_count >= VICTIM_DEMOTION) {
#ifdef DEBUG
						printf("[%s] page %d moved wait_list\n",
						       __func__, p->num);
#endif
						list_del_init(&p->mq);
						list_move_tail(&p->wait, wait_list);
					}
				}
			}
		}
	}
}


void aging(struct dram_frame *DRAM, struct pcm_frame *PCM, struct page *p, int op,
	   struct list_head *victim_list, struct mqvec *mqvec_pcm,
	   struct mqvec *mqvec_dram, struct list_head *wait_list, int demote_period)
{
	int i;
	for_each_dram(i) {
		if (DRAM->page[i] != NULL) {
			if (DRAM->page[i] == p)
				continue;

			DRAM->page[i]->demote_count++;
			if (DRAM->page[i]->demote_count >= demote_period){
				DRAM->page[i]->frq--;
				DRAM->page[i]->demote_count = 0;
				if (DRAM->page[i]->frq <= 0) {
					DRAM->page[i]->frq = 0;
				}
				check_demote(DRAM, PCM, 1, DRAM->page[i],
					     victim_list, mqvec_dram, wait_list);
			}
		}
	}

	for_each_pcm(i) {
		if (PCM->page[i] != NULL) {
			if (PCM->page[i] == p)
				continue;

			PCM->page[i]->demote_count++;
			if (PCM->page[i]->demote_count >= demote_period){
				PCM->page[i]->frq--;
				PCM->page[i]->demote_count = 0;
				if (PCM->page[i]->frq <= 0) {
					PCM->page[i]->frq = 0;
				}
				check_demote(DRAM, PCM, 0, PCM->page[i],
					     victim_list, mqvec_pcm, wait_list);
			}
		}
	}
}

/*
 * base algorithm is LRU. we first input to the PCM frame.
 * if operation is write, we input multi queue.
 * and check it to promotion.
 */
void my_mq_algorithm(struct dram_frame *DRAM, struct pcm_frame *PCM, struct page *p, int op,
		     struct stat *stat, struct mqvec *mqvec_pcm, struct mqvec *mqvec_dram,
		     struct list_head *victim_list, struct list_head *wait_list,
		     int mq_migrate_th, int demote_period)
{
	int i,j;
	int s,s2;
	int frame_num;
	int isDRAM = 1;
	struct page *dram;
	struct page *pcm;

	if (p->isDRAM == 1) {
		if (op == R) {
#ifdef DEBUG
			printf("Read %d : DRAM HIT\n", p->num);
#endif
			stat->dram_read_hit++;
			handle_read_op(DRAM, PCM, 1, p, mqvec_pcm, mqvec_dram,
				       victim_list, wait_list, stat, mq_migrate_th);
		} else {
#ifdef DEBUG
			printf("Write %d : DRAM HIT\n", p->num);
#endif
			stat->dram_write_hit++;
			handle_write_op(DRAM, PCM, 1, p, mqvec_pcm, mqvec_dram,
					victim_list, wait_list, stat, mq_migrate_th);
		}
	} else if (p->isDRAM == 0) {
		if (op == R) {
#ifdef DEBUG
			printf("Read %d : PCM HIT\n", p->num);
#endif
			stat->pcm_read_hit++;
			handle_read_op(DRAM, PCM, 0, p, mqvec_pcm, mqvec_dram,
				       victim_list, wait_list, stat, mq_migrate_th);
		} else {
#ifdef DEBUG
			printf("Write %d : PCM HIT\n", p->num);
#endif

			//migration_to_dram(DRAM, PCM, p, victim_list, wait_list,
			//		  stat, mqvec_dram, p->level);

			stat->pcm_write_hit++;
			handle_write_op(DRAM, PCM, 0, p, mqvec_pcm, mqvec_dram,
				victim_list, wait_list, stat, mq_migrate_th);

		}

	/*
	 * DRAM & PCM miss.
	 */
	} else {
		/*
		if (op == W) {
			s = is_free_dram(DRAM);
			if (s == -1) {
				dram = select_victim(victim_list, mqvec_dram, isDRAM);

				s2 = is_free_pcm(PCM);
				if( s2 == -1) {
					pcm = select_victim(wait_list, mqvec_pcm, !isDRAM);
					frame_num = write_back(PCM, pcm, stat);

					frame_num = migration_to_pcm(PCM, dram,
								     frame_num,
								     wait_list, stat);
					set_to_dram(DRAM, p, frame_num, mqvec_dram);

				} else {
					frame_num = migration_to_pcm(PCM, dram,
								     s2, wait_list, stat);
					set_to_dram(DRAM, p, frame_num, mqvec_dram);
				}
			} else {
				set_to_dram(DRAM, p, s, mqvec_dram);
			}
		} else {
		*/
			s = is_free_pcm(PCM);
			if( s == -1) {
				pcm = select_victim(wait_list,
						    mqvec_pcm, !isDRAM);
				frame_num = write_back(PCM, pcm, stat);
				set_to_pcm(PCM, p, frame_num, mqvec_pcm);

			} else {
				set_to_pcm(PCM, p, s, mqvec_pcm);
			}
		//}

		if (op == R) {
#ifdef DEBUG
			printf("Read %d : MISS!!\n", p->num);
#endif
			stat->read_miss++;
		} else {
#ifdef DEBUG
			printf("Write %d : MISS!!\n", p->num);
#endif
			p->dirty = 1;
			stat->write_miss++;
		}
		p->ref = 1;
		p->frq++;
	}

	//aging(DRAM, PCM, p, op, victim_list, mqvec_pcm, mqvec_dram, wait_list, demote_period);
}

void simulation_my_mq(char *input_file, struct dram_frame *DRAM,
			  struct pcm_frame *PCM, struct page *page, struct stat *stat,
			  struct mqvec *mqvec_pcm, struct mqvec *mqvec_dram,
			  struct list_head *victim_list, struct list_head *wait_list,
			  int mq_migrate_th, int demote_period)
{
	int i=0;
	FILE *f = fopen(input_file, "r");
	int check=0;
	char buf[BUF_SIZE];
	struct input input;
//	int count = 100000;

	if (f != NULL) {
		while(fgets(buf, sizeof(buf), f) != NULL) {
			i++;

			init_input(&input);
			if (token_split(&input, buf)){
//				count--;
//				if (count == 0) {
//					printf("\n============ %d th ============", i);
//					count = 100000;
//				}
#ifdef DEBUG
				printf("\n============ %d th ============", i);
				printf("\n==========[ %c, %d ]========\n",
		   		    input.type == 0 ? 'R' : 'W', input.num);
#endif
				my_mq_algorithm(DRAM, PCM, &page[input.num], input.type,
						stat, mqvec_pcm, mqvec_dram, victim_list, wait_list,
						mq_migrate_th, demote_period);
#ifdef DEBUG
				print_RAMS(DRAM, PCM);
				print_victim_list(victim_list);
				print_my_mq(mqvec_dram);
				print_wait_list(wait_list);
				print_my_mq(mqvec_pcm);
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
	printf("#################################################\n");
#endif
}
