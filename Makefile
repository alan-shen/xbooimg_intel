CC=gcc
BINNAME=xbootimg
MAINFILE=xbootimg.c
TIME=`date`

all:
	@rm -f timestamp.c
	@echo "char DATE[] = \"`date +%Y/%m/%d-%H:%M:%S`\";" > timestamp.c
	$(CC) $(MAINFILE) timestamp.c -o $(BINNAME)
	@rm -f timestamp.c

clean:
	@rm xbootimg
