all: floating array

floating: floating.c
	gcc floating.c -ftree-vectorize -O3 -msse2 -o floating
array: array.c
	gcc array.c -O3 -lm -o array

clean:
	rm array floating
