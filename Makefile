COMPILER=cc

C = c
OUTPUT_PATH = bin/
SOURCE_PATH = src/
BIN = bin/mbpfan
CONF = mbpfan.conf
DOC = README.md
MAN = mbpfan.8.gz

COPT = 
CC = cc
OBJFLAG = -o
BINFLAG = -o
INCLUDES =
LIBS = -lm
LIBPATH =
CFLAGS +=  $(COPT) -g $(INCLUDES) #-Wall
LDFLAGS += $(LIBPATH) -g $(LIBS) #-Wall
DEP = dep


OBJS := $(patsubst %.$(C),%.$(OBJ),$(wildcard $(SOURCE_PATH)*.$(C)))

%.$(OBJ):%.$(C)
	mkdir -p bin
	@echo Compiling $(basename $<)...
	$(CC) -c $(CFLAGS) $< $(OBJFLAG)$@

all: $(OBJS)
	@echo Linking...
	$(CC) $(LDFLAGS) $^ $(LIBS) $(BINFLAG) $(BIN)

clean:
	rm -rf $(SOURCE_PATH)*.$(OBJ) $(BIN)

tests:
	make install
	/usr/sbin/mbpfan -f -v -t

uninstall:
	rm /usr/sbin/mbpfan
	rm /etc/mbpfan.conf
	rm /lib/systemd/system/mbpfan.service
	rm /usr/share/man/man8/mbpfan.8.gz
	rm -rf /usr/share/doc/mbpfan

install:
	make
	install -d $(DESTDIR)/usr/sbin
	install -d $(DESTDIR)/etc
	install -d $(DESTDIR)/lib/systemd/system
	install -d $(DESTDIR)/usr/share/doc/mbpfan
	install $(BIN) $(DESTDIR)/usr/sbin
	install -m644 $(CONF) $(DESTDIR)/etc
	install -m644 $(DOC) $(DESTDIR)/usr/share/doc/mbpfan
	install -d $(DESTDIR)/usr/share/man/man8
	install -m644 $(MAN) $(DESTDIR)/usr/share/man/man8
	@echo ""
	@echo "******************"
	@echo "INSTALL COMPLETED"
	@echo "******************"
	@echo ""
	@echo "A configuration file has been copied (might overwrite existing file) to /etc/mbpfan.conf."
	@echo "See README.md file to have mbpfan automatically started at system boot."
	@echo ""
	@echo "Please run the tests now with the command"
	@echo "   sudo make tests"
	@echo ""
rebuild: clean all
#rebuild is not entirely correct
