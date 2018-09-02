all:
	clear
	mpicc main.c pimglib.c  -lm
	mpiexec -n 4 ./a.out -f ./waterfall/waterfall_1920_2520.raw -h 2520 -w 1920 -c rgb
