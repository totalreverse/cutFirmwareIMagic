
all:	testIMagic

testIMagic:	testIMagic.c
	$(CC) -O2 testIMagic.c -static -o testIMagic -lusb


