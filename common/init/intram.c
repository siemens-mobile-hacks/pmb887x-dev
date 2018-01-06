extern unsigned int _cpu_vectors;

void __init_vectors() {
	// Очищаем зачем-то первый бит у SCU_ROMAMCR
	// Иначе не работает запись векторов o_O
	(*(volatile unsigned int *) (0xf440007C)) &= ~1;
	
	void **vectors = (void **) 0;
	for (int i = 0; i < 16; ++i)
		vectors[i] = (&_cpu_vectors)[i];
}
