AXIS_USABLE_LIBS = UCLIBC GLIBC
include $(AXIS_TOP_DIR)/tools/build/rules/common.mak
include $(AXIS_TOP_DIR)/tools/build/rules/recurse.mak

SAFIE_BUILD_TARGET=$(AXIS_BUILDTYPE)
SAFIE_CONFIG_HOST=$(SAFIE_BUILD_TARGET)

PROGS     = axisptztest

PKGS = glib-2.0 gio-2.0 fixmath axptz
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_LIBDIR) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_LIBDIR) pkg-config --libs $(PKGS))
LDLIBS += -lcapture

SRCS      = $(CURDIR)/main.cpp $(CURDIR)/AxisPtzApi.cpp
OBJS      = $(SRCS:.c=.o)

SAFIE_LDFLAGS += -I$(AXIS_TOP_DIR)/target/$(AXIS_BUILDTYPE)/usr/include/fixmath
#SAFIE_LDFLAGS += -I$(AXIS_TOP_DIR)/target/$(AXIS_BUILDTYPE)/usr/include/glib-2.0
#SAFIE_LDFLAGS += `pkg-config --cflags glib-2.0`

SAFIE_CXXFLAGS = $(CXXFLAGS) -Wno-shadow -pthread
SAFIE_LDFLAGS += -Wl,--as-needed,-rpath-link=$(SAFIE_EXT_LIB_DIR)/lib -L$(SHLIB_DIR) -L$(SAFIE_EXT_LIB_DIR)/lib -Wl,-R,'$$ORIGIN/lib' $(LDFLAGS)
SAFIE_LIBS  += -lcapture -lparam -levent -lnet_http -Wl,-Bstatic,-llicensekey_stat,-Bdynamic,-llicensekey
SAFIE_LIBS  += $(LDLIBS)
#SAFIE_LIBS  += -lpthread -ldl -lresolv -lrt
#SAFIE_LIBS	+= -llivemedia

SAFIE_LDFLAGS  += -rdynamic 

.PHONY: all clean

all: $(PROGS)

$(PROGS): $(OBJS)
	$(CXX) $(CFLAGS) $(SAFIE_LDFLAGS) $^ $(SAFIE_LIBS) -o $@
#$(PROGS): $(OBJS)
#	$(CXX) $(LDFLAGS) $^ $(LIBS) $(LDLIBS) -o $@

clean:
	rm -f $(PROGS) *.o




