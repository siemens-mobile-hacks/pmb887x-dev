all:
	gcc -shared -ldl strange.c -o wine_term_hack32.so -m32 -I/usr/include/x86_64-linux-gnu
	gcc -shared -ldl strange.c -o wine_term_hack64.so -m32 -I/usr/include/x86_64-linux-gnu
clean:
	rm *.o *.so
