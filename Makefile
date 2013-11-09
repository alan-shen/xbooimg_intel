CC=gcc
BINNAME=xbootimg
MAINFILE=xbootimg.c
TIME=`date`
#FLAGS=--static

all:
	@rm -f timestamp.c
	@echo "char DATE[] = \"`date +%Y/%m/%d-%H:%M:%S`\";" > timestamp.c
	$(CC) $(MAINFILE) timestamp.c -o $(BINNAME) ${FLAGS}
	@rm -f timestamp.c

clean:
	@rm xbootimg
