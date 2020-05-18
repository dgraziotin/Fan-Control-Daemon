COMPILER=cc

C = c
OBJ = o
OUTPUT_PATH = bin/
SOURCE_PATH = src/
BIN = bin/mbpfan
CONF = mbpfan.conf
CONF_TEST_0 = mbpfan.conf.test0
CONF_TEST_1 = mbpfan.conf.test1
CONF_TEST_2 = mbpfan.conf.test2
DOC = README.md
MAN = mbpfan.8.gz

COPT = 
CC ?= cc
OBJFLAG = -o
BINFLAG = -o
INCLUDES =
LIBS = -lm
LIBPATH =
CFLAGS += $(COPT) -g $(INCLUDES) -Wall -Wextra -Wno-unused-function -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500
LDFLAGS += $(LIBPATH) -g

OBJS := $(patsubst %.$(C),%.$(OBJ),$(wildcard $(SOURCE_PATH)*.$(C)))

%.$(OBJ):%.$(C)
	mkdir -p bin
	@echo Compiling $(basename $<)...
	$(CC) -c $(CFLAGS) $< $(OBJFLAG)$@

all: $(BIN)

$(BIN): $(OBJS)
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
	rm -rf /etc/mbpfan.d
	rm /lib/systemd/system/mbpfan.service
	rm /usr/share/man/man8/mbpfan.8.gz
	rm -rf /usr/share/doc/mbpfan

install: $(BIN)
	install -d $(DESTDIR)/usr/sbin
	install -d $(DESTDIR)/etc/mbpfan.d
	install -d $(DESTDIR)/lib/systemd/system
	install -d $(DESTDIR)/usr/share/doc/mbpfan
	install $(BIN) $(DESTDIR)/usr/sbin
	install -m644 $(CONF) $(DESTDIR)/etc
	install -Tm644 $(CONF) $(DESTDIR)/etc/mbpfan.d/$(CONF).sample
	install -m644 $(CONF_TEST_0) $(DESTDIR)/etc/mbpfan.d
	install -m644 $(CONF_TEST_1) $(DESTDIR)/etc/mbpfan.d
	install -m644 $(CONF_TEST_2) $(DESTDIR)/etc/mbpfan.d
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
