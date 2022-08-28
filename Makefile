bsp2: bsp2.c
	$(CC) -o $@ -Wall $<

clean:
	rm -f bsp2

.PHONY: clean
