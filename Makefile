
high_perf_calc.o:
	gcc -o high_perf_calc.o -c high_perf_calc.c

delete_c: high_perf_calc.o
	rm -f high_perf_calc.c

hpc: high_perf_verification.c delete_c
	gcc -Wall -g high_perf_calc.o high_perf_verification.c -o hpc

clean:
	rm -f hpc
