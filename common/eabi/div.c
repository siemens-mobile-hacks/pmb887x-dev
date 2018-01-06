
// AEABI
unsigned int __udivmodsi4(unsigned int num, unsigned int den, unsigned int * rem_p) {
	unsigned int quot = 0, qbit = 1;
	
	/* Left-justify denominator and count shift */
	while ((int) den >= 0) {
		den <<= 1;
		qbit <<= 1;
	}

	while (qbit) {
		if (den <= num) {
			num -= den;
			quot += qbit;
		}
		den >>= 1;
		qbit >>= 1;
	}

	if (rem_p)
		*rem_p = num;

	return quot;
}

signed int __aeabi_idiv(signed int num, signed int den) {
	signed int minus = 0;
	signed int v;
	
	if (num < 0) {
		num = -num;
		minus = 1;
	}
	if (den < 0) {
		den = -den;
		minus ^= 1;
	}
	v = __udivmodsi4(num, den, 0);
	if (minus)
		v = -v;
	return v;
}

unsigned int __aeabi_uidiv(unsigned int num, unsigned int den) {
	return __udivmodsi4(num, den, 0);
}
