char ___stack_top[512 * 1024];
char ___irq_stack_top[512 * 1024];

extern unsigned int _cpu_vectors;

void __init_vectors() {
	void **vectors = (void **) 0;
	for (int i = 0; i < 16; ++i)
		vectors[i] = (void *) (&_cpu_vectors)[i];
}
