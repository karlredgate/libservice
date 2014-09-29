
INCLUDES = -I../xen/vendor/dist/install/usr/include
CFLAGS += -fpic -g $(INCLUDES)
CXXFLAGS += -fpic -g $(INCLUDES)

MAJOR_VERSION=1
MINOR_VERSION=0
REVISION=42284

LINKNAME=libservice.so
SONAME=$(LINKNAME).$(MAJOR_VERSION)
REALNAME=$(SONAME).$(MINOR_VERSION).$(REVISION)

RPM_DIR=$(KIT_RPM_DIR)/dom0/base

default: build install
kit: build test install
build: $(UTILTARGET) all test

all: $(SONAME)
	# $(MAKE) -C src

OBJS = \
	BIOS.o \
	Service.o \
	Channel.o \
	StringList.o \
	Thread.o \
	Hypercall.o \
	Xen.o \
	XenStore.o \
	Kernel.o \
	UUID.o \
	util.o

CLEANS += $(SONAME)
$(SONAME): $(OBJS)
	$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^ -lc -ltcl
	rm -f $(LINKNAME)
	ln -s $(SONAME) $(LINKNAME)

Hypercall.o: Hypercall.cc Hypercall.h
	$(CXX) $(CXXFLAGS) -c $<

Xen.o: Xen.cc Hypercall.h
	$(CXX) $(CXXFLAGS) -c $<

XenStore.o: XenStore.cc XenStore.h
	$(CXX) $(CXXFLAGS) -c $<

Kernel.o :: Kernel.h
BIOS.o :: BIOS.h
Service.o :: Service.h Thread.h
Channel.o :: Channel.h Service.h Thread.h
StringList.o :: StringList.h
Thread.o :: Thread.h
UUID.o :: UUID.h
util.o :: util.h

test:
	echo "Run unit tests here"

install: rpm
	install --directory --mode 755 $(RPM_DIR)
	rm -f $(RPM_DIR)/redgate-libservice-*.rpm
	cp rpm/RPMS/*/redgate-libservice-*.rpm $(RPM_DIR)/

uninstall:
	rm -f $(RPM_DIR)/redgate-libservice-*.rpm

dist: build
	$(RM) -rf exports
	mkdir -p exports
	$(INSTALL) -d --mode=755 exports/usr/lib
	$(INSTALL) --mode=755 $(SONAME) exports/usr/lib/$(REALNAME)
	ln -s $(REALNAME) exports/usr/lib/$(SONAME)
	ln -s $(REALNAME) exports/usr/lib/$(LINKNAME)
	$(INSTALL) -d --mode=755 exports/usr/share/man/man3
	$(INSTALL) Service.3 exports/usr/share/man/man3

rpm: dist
	rm -rf rpm
	mkdir -p rpm/BUILD rpm/RPMS rpm/BUILDROOT
	rpmbuild -bb --buildroot=$(TOP)platform/libservice/rpm/BUILDROOT libservice.spec

clean:
	$(RM) *.o $(OBJS) $(CLEANS)
	$(RM) -rf rpm exports

distclean: uninstall clean

.PHONY: test
