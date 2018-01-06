extern unsigned int _cpu_vectors;

void __init_vectors() {
	int i;
	void **vectors = (void **) 0;
	for (i = 0; i < 8; ++i)
		vectors[i] = (void *) (&_cpu_vectors)[i];
}
