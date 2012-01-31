
hpc: high_perf_calc.c high_perf_verification.c
		gcc -Wall -g high_perf_calc.c high_perf_verification.c -o hpc

clean:
	rm -f hpc
