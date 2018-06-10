COMPILER=cc

C = c
OBJ = o
OUTPUT_PATH = bin/
SOURCE_PATH = src/
VERSION := 2.0.2
TMP_DIR := $(shell mktemp -d)
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
CFLAGS +=  $(COPT) -g $(INCLUDES) -Wall -Wextra -Wno-unused-function
LDFLAGS += $(LIBPATH) -g $(LIBS) #-Wall
FILES   := $(wildcard src/*) $(wildcard mbpfan.*) Makefile

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
	rm -rf $(SOURCE_PATH)*.$(OBJ) $(BIN) mbpfan-$(VERSION).tar.gz

tests:
	make install
	/usr/sbin/mbpfan -f -v -t

uninstall:
	rm /usr/sbin/mbpfan
	rm /etc/mbpfan.conf
	rm /lib/systemd/system/mbpfan.service
	rm /usr/share/man/man8/mbpfan.8.gz
	rm -rf /usr/share/doc/mbpfan

install: $(BIN)
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

mbpfan-$(VERSION).tar.gz: clean $(FILES)
	mkdir $(TMP_DIR)/mbpfan-$(VERSION)
	cp -r * $(TMP_DIR)/mbpfan-$(VERSION)
	cd $(TMP_DIR) ; \
	tar cfz $(TMP_DIR)/mbpfan-$(VERSION).tar.gz mbpfan-$(VERSION)
	mv $(TMP_DIR)/mbpfan-$(VERSION).tar.gz .
	rm -rf $(TMP_DIR)

rpms: mbpfan-$(VERSION).tar.gz
	mkdir $(TMP_DIR)
	mkdir $(TMP_DIR)/BUILD
	mkdir $(TMP_DIR)/RPMS
	mkdir $(TMP_DIR)/SOURCES
	mkdir $(TMP_DIR)/SRPMS
	cp mbpfan-$(VERSION).tar.gz $(TMP_DIR)/SOURCES
	cd $(TMP_DIR)/BUILD ; \
	tar xfz $(TMP_DIR)/SOURCES/mbpfan-$(VERSION).tar.gz \
		mbpfan-$(VERSION)/mbpfan.spec
	rpmbuild --define '_topdir $(TMP_DIR)' \
		 -ba $(TMP_DIR)/BUILD/mbpfan-$(VERSION)/mbpfan.spec
	mv $(TMP_DIR)/RPMS/x86_64/mbpfan-$(VERSION)-*.x86_64.rpm .
	rm -rf $(TMP_DIR)
