ALLFILES		=	main.cpp generator.cpp random.cpp syntax_tree.cpp util.cpp

CC	    =   g++
FLAG    = -I./  -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS -fpermissive -fsanitize=address -fno-omit-frame-pointer
DEBUGFLAG   =	-D DEBUG -g -pg
RELEASEFLAG = 	-O3

btor2fuzz :	release

.PHONY :    release debug clean

release :   $(ALLFILES)
	    $(CC) $(FLAG) $(RELEASEFLAG) $(ALLFILES) -lm -o btor2fuzz

debug :	$(ALLFILES)
	$(CC) $(FLAG) $(DEBUGFLAG) $(ALLFILES) -lm -o btor2fuzz

clean :
	rm -f *.o btor2fuzz
