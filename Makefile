
PACKAGE = libservice
PWD := $(shell pwd)
DEPENDENCIES = tcl

MAJOR_VERSION=1
MINOR_VERSION=3
REVISION=42284

LIBRARY_NAME = libservice

OBJS =
OBJS += string_util.o
OBJS += tcl_util.o
OBJS += StringList.o
OBJS += UUID.o
OBJS += TCL_UUID.o
OBJS += Channel.o
OBJS += TCL_Channel.o
OBJS += AppInit.o
OBJS += TCL_Thread.o
OBJS += Thread.o
OBJS += TCL_Thread.o
OBJS += Service.o
# OBJS += Hypercall.o
# OBJS += Xen.o
# OBJS += XenStore.o
# OBJS += Kernel.o
OBJS += SMBIOS.o
# OBJS += util.o
OBJS += xuid.o
OBJS += traps.o
OBJS += syslog_logger.o
OBJS += Allocator.o
OBJS += TCL_Allocator.o

default: all test

OS := $(shell uname -s)
include Makefiles/$(OS).mk
# For package platform specific includes
INCLUDES += -I$(OS)/include
INCLUDES += -I.

CFLAGS += -fpic -g $(INCLUDES)
CXXFLAGS += -fpic -g $(INCLUDES)


kit: build test install
build: $(UTILTARGET) all test

# all: dependencies $(SONAME)
all: dependencies sonar
	# $(MAKE) -C src

dependencies: distro_dependencies
	@echo "No generic dependencies"

sonar: sonar.o $(LIBRARY_TARGET)
	$(CXX) -o $@ $^ -lstdc++ $(LDFLAGS) -ltcl

CLEANS += $(LIBRARY_TARGET) $(LINKNAME)
$(LIBRARY_TARGET): $(OBJS)
	$(CXX) $(SHARED_LIB_FLAGS) -o $@ $^ -lc $(LDFLAGS) -ltcl
	: rm -f $(LINKNAME)
	: ln -s $(LIBRARY_TARGET) $(LINKNAME)

Hypercall.o: Hypercall.cc Hypercall.h
	$(CXX) $(CXXFLAGS) -c $<

Xen.o: Xen.cc Hypercall.h
	$(CXX) $(CXXFLAGS) -c $<

XenStore.o: XenStore.cc XenStore.h
	$(CXX) $(CXXFLAGS) -c $<

Kernel.o :: Kernel.h
smbios.o :: smbios.h
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
