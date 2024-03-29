ALLFILES	=	main.cpp generator.cpp random.cpp syntax_tree.cpp util.cpp

CC	    	=   g++
FLAG    	= -I./  -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS #-fsanitize=address -fno-omit-frame-pointer -fpermissive
RELEASEFLAG = 	-O3
DEBUGFLAG   =	-D DEBUG -g -pg

fuzzbtor2	:	release

.PHONY 		:	release debug clean

release		:	$(ALLFILES)
			$(CC) $(FLAG) $(RELEASEFLAG) $(ALLFILES) -lm -o fuzzbtor2 -std=c++11

debug		:	$(ALLFILES)
			$(CC) $(FLAG) $(DEBUGFLAG) $(ALLFILES) -lm -o fuzzbtor2 -std=c++11

clean		:
			rm -f *.o fuzzbtor2
