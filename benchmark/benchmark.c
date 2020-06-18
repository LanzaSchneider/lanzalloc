#include "../lanzalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#define POOL_SIZE	0x100000

int main( int argc, char** argv ) {
	uint8_t* POOL = (uint8_t*)malloc(POOL_SIZE);
	lanzalloc_size_t bench_num = 100000;
	lanzalloc_size_t block_max = POOL_SIZE / bench_num;
	lanzalloc_size_t* size_list = (lanzalloc_size_t*)calloc(sizeof(lanzalloc_size_t), bench_num);
	lanzalloc_size_t i;
	for ( i = 0; i < bench_num; i++ ) size_list[i] = rand() % block_max;
	printf("Benchmark\n");
	{
		printf("Malloc ");
		#define alloc(s)	malloc(s)
		#define fr(p)		free(p)
		#include "benchmark_func.h"
	}
	{
		struct lanzalloc* lanzalloc = lanzalloc_initialize(POOL, POOL_SIZE, 5);
		printf("Lanzalloc ");
		#define alloc(s)	lanzalloc_alloc(lanzalloc, s)
		#define fr(p)		lanzalloc_free(lanzalloc, p)
		#include "benchmark_func.h"
	}
	free(size_list);
	free(POOL);
	return 0;
}
