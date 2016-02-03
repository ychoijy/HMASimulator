#include <map>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 40
#define INPUT_SIZE	1000000
#define R 0;
#define W 1

using namespace std;

#if 0
#define SUBSET		1000000
#define SUBSET_RANGE	200000
#endif

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

bool comp(struct access_information a, struct access_information b) {
	return (a.num < b.num);
}

void get_access_count(map<long, struct access_information> *info_map) {
	int w_count=0, r_count=0;
	FILE *access_count = fopen("./access_count.dat", "w");
	map<long, struct access_information>::iterator Iter;
	vector<struct access_information> vec;
	vector<struct access_information>::iterator vec_iter;

	for (Iter = info_map->begin(); Iter != info_map->end(); Iter++) {
		vec.push_back(Iter->second);
	}

	sort(vec.begin(), vec.end(), comp);

	for (vec_iter = vec.begin() ; vec_iter != vec.end(); vec_iter++) {
		fprintf(access_count, "%d\t%d\t%d\t%d\n", vec_iter->num, vec_iter->overlab_count,
				vec_iter->w_count, vec_iter->r_count);
		w_count += vec_iter->w_count;
		r_count += vec_iter->r_count;
	}
	printf("================= INPUT INFO ==================\n");
	printf("total : %d, Write : %d, Read : %d\n", w_count + r_count, w_count, r_count);
	printf("===============================================\n");
	fclose(access_count);
}

void get_access_pattern(FILE *access_pattern, struct input *input, int loop_count)
{
	int i, num, j=0;
	for (i=0; i<INPUT_SIZE && input[i].num != -100; i++) {
		num = i + (loop_count * INPUT_SIZE);

#ifdef SUBSET
		if (num >= SUBSET && num < SUBSET+SUBSET_RANGE){
			if (input[i].type == W) {
				fprintf(access_pattern, "%d %d %d\n", num-SUBSET, input[i].num, 0);
			} else {
				fprintf(access_pattern, "%d %d %d\n", num-SUBSET, 0, input[i].num);
			}
		}
#else
		if (input[i].type == W) {
			fprintf(access_pattern, "%d %d %d\n", num, input[i].num, 0);
		} else {
			fprintf(access_pattern, "%d %d %d\n", num, 0, input[i].num);
		}
#endif
	}

}


int overlab_check(struct input *input, map<long, struct access_information> *info_map, int count)
{
	int i,j;
	map<long, struct access_information>::iterator FindIter;


	for (i=0; i<INPUT_SIZE && input[i].address != -100; i++) {
		FindIter = info_map->find(input[i].address);

		if (FindIter != info_map->end()) {
			input[i].num = FindIter->second.num;
			FindIter->second.overlab_count++;
			if (input[i].type == W) {
				FindIter->second.w_count++;
			} else {
				FindIter->second.r_count++;
			}

		} else {
			struct access_information info;

			info.address = input[i].address;
			info.num = count;
			input[i].num = count;
			count++;
			info.overlab_count = 1;
			if (input[i].type == W) {
				info.w_count = 1;
				info.r_count = 0;
			} else {
				info.r_count = 1;
				info.w_count = 0;
			}
			info_map->insert(map<long, struct access_information>::value_type(input[i].address, info));
		}
	}
	printf("i = %d\n", i);
	return count;
}
int token_split(struct input *input, char *buf)
{
	char *ptr, *stop;
	int i=0;
	int count = 0;

	ptr = strtok(buf, " ");


	if (strcmp(ptr, "MISS")) {
		return 0;
	}

	//strcpy(input->operation, ptr);

	while ((ptr = strtok(NULL, " ")) != NULL) {
		count++;

		if (count >= 3) {
			return 0;
		}
		if (i == 0) {
			if (ptr[0] == 'W' ) {
				input->type = W;
			} else if(ptr[0] == 'R') {
				input->type = R;
			} else {
				return 0;
			}
			i++;
		} else {
			//input->address = strtol(ptr, &stop, 16);
			input->address = atol(ptr);
			if (input->address <= 0) {
				return 0;
			}
		}
	}

	if (count != 2 ) {
		return 0;
	}

	return 1;
}

int get_input(FILE *f, struct input *input)
{
	int i=0;
	int check=0;
	char buf[BUF_SIZE];

	if (f != NULL) {
		for( i=0;i<INPUT_SIZE;i++){
			if (fgets(buf, sizeof(buf), f) == NULL) {
				return 1;
			}
			if(!token_split(&input[i], buf)) {
				i--;
			}
		}
	} else {
		printf("File empty\n");
	}
	return 0;
}

void init_input(struct input *input)
{
	int i;
	for (i=0;i<INPUT_SIZE;i++){
		input[i].num = -100;
		input[i].type = -100;
		input[i].address = -100;
	}
}

int main(int argc, char *argv[])
{
	int page_num=0, loop_count=0, isEnd=0;
	struct input *input;
	FILE *result_file;

	map<long, struct access_information> info_map;
	input = (struct input *)malloc(sizeof(struct input)*INPUT_SIZE);

#ifdef SUBSET
	FILE *access_pattern = fopen("./subset.dat", "w");
#else
	FILE *access_pattern = fopen("./access_pattern.dat", "w");
#endif
	FILE *f = fopen(argv[1], "r");

	while(!isEnd && page_num != -1) {
		printf("loop_count = %d\n", loop_count);
		init_input(input);

		isEnd = get_input(f, input);

		page_num = overlab_check(input, &info_map, page_num);

		printf("page_num = %d\n", page_num);

		get_access_pattern(access_pattern, input, loop_count++);
	}
	printf("end\n");
	get_access_count(&info_map);
	fclose(f);
	fclose(access_pattern);
	free(input);
	return 0;
}
