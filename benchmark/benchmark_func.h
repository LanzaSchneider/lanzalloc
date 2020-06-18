{
	void** ptr_list = (void*)calloc(sizeof(void*), bench_num);
	lanzalloc_size_t i;
	clock_t start = clock();
	for ( i = 0; i < bench_num; i++ ) {
		ptr_list[i] = alloc(size_list[i]);
	}
	printf("alloc: %.7f ms ", (clock() - start) / 1000.0f * CLOCKS_PER_SEC);
	for ( i = 0; i < bench_num; i++ ) {
		fr(ptr_list[i]);
	}
	printf("free: %.7f ms\n", (clock() - start) / 1000.0f * CLOCKS_PER_SEC);
	free(ptr_list);
}

#undef alloc
#undef fr
