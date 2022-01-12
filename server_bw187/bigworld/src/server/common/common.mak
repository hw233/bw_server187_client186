# Thread Local Storage is broken in redhat8 (with a non-tls ld-linux.so) and
# Debian (up to and including Sarge).
LDLINUX_TLS_IS_BROKEN = 1

ifndef MF_CONFIG
	MF_CONFIG=Hybrid
endif

# This variable is used by src/lib/python/configure to determine whether to
# print out nasty error messages.
export BUILDING_BIGWORLD=1

# ifndef PYTHONPREFIX
# 	PYTHONPREFIX=/usr/local
# endif

PYTHONLIB=python2.5

ifeq (,$(findstring $(MF_CONFIG), Release Hybrid Debug Evaluation))
Error - Unknown configuration type
endif

LIBDIR = $(MF_ROOT)/lib/$(MF_CONFIG)

ifneq (,$(findstring s, $(MAKEFLAGS)))
QUIET_BUILD=1
endif

# If SEPARATE_DEBUG_INFO is defined, the debug information for an executable
# will be placed in a separate file. For example, cellapp and cellapp.dbg. The
# majority of the executable's size is debug information.
# SEPARATE_DEBUG_INFO=1

# This file is used for somewhat of a hack. We want to display a line of info
# the first time a .o is made for a component (and not display if no .o files
# are made.
MSG_FILE = make$(MAKELEVEL).tmp

ifdef BIN
ifndef INSTALL_DIR
	OUTPUTDIR = $(MF_ROOT)/bigworld/bin/$(MF_CONFIG)
else

# INSTALL_ALL_CONFIGS has been put in to be used by unit_tests so the Debug
# and Hybrid binaries are both placed in MF_ROOT/tests/MF_CONFIG not just
# the Hybrid builds.
ifdef INSTALL_ALL_CONFIGS
	OUTPUTDIR = $(INSTALL_DIR)/$(MF_CONFIG)
else
# For the tools, the Hybrid configuration is automatically made into the install
# directory. Other configurations are made locally.
ifeq ($(MF_CONFIG), Hybrid)
	OUTPUTDIR = $(INSTALL_DIR)
else
	OUTPUTDIR = $(MF_CONFIG)
endif
endif
endif # INSTALL_ALL_CONFIGS

	OUTPUTFILE = $(OUTPUTDIR)/$(BIN)
endif

ifdef SO
	OUTPUTDIR = $(MF_ROOT)/bigworld/bin/$(MF_CONFIG)/$(COMPONENT)-extensions
	OUTPUTFILE = $(OUTPUTDIR)/$(SO).so
endif

ifdef LIB
	OUTPUTDIR = $(LIBDIR)
	OUTPUTFILE = $(OUTPUTDIR)/lib$(LIB).a
endif

#----------------------------------------------------------------------------
# Macros
#----------------------------------------------------------------------------

# Our source files
OUR_CPP = $(addsuffix .cpp, $(SRCS))
OUR_ASMS = $(addsuffix .s, $(ASMS))
ALL_SRC = $(SRCS) $(ASMS)

# All .o files that need to be linked
OBJS = $(addsuffix .o, $(ALL_SRC))

# Standard libs that everyone gets
# don't want these for a shared object - we'll use the exe's instead
ifndef SO
ifndef NO_EXTRA_LIBS
MY_LIBS += network resmgr zip math cstdmf
endif
endif

# Include and lib paths
LDFLAGS += -L$(LIBDIR)
CPPFLAGS += -I $(MF_ROOT)/src/lib/python/Include
CPPFLAGS += -I $(MF_ROOT)/src/lib
CPPFLAGS += -I $(MF_ROOT)/bigworld/src
CPPFLAGS += -I $(MF_ROOT)/bigworld/src/server

LDLIBS += $(addprefix -l, $(MY_LIBS))

# everyone needs pthread if LDLINUX_TLS_IS_BROKEN
ifdef LDLINUX_TLS_IS_BROKEN
CPPFLAGS += -DLDLINUX_TLS_IS_BROKEN
LDLIBS += -lpthread
endif

ifdef USE_PYTHON
LDFLAGS += -export-dynamic
LDFLAGS += -L$(MF_ROOT)/src/lib/python
LDLIBS += -l$(PYTHONLIB) -lpthread -lutil -ldl
endif

ifdef USE_OPENSSL
OPENSSL_DIR = $(MF_ROOT)/src/lib/third_party/openssl
CPPFLAGS += -I$(OPENSSL_DIR)/include
LDFLAGS += -L$(OPENSSL_DIR)
LDLIBS += -lssl -lcrypto -ldl
CPPFLAGS += -DUSE_OPENSSL
endif

# Use backwards compatible hash table style. This is because Fedora Core 6
# defaults to using "gnu" style hash tables which produces incompatible
# binaries with FC5 and before.
#
# By setting it to "both", we can have advantages of the faster hash style
# on FC6 systems and backwards compatibilities with older systems, at a
# small size penalty which is < 0.1% of file size.
ifneq ($(shell gcc -dumpspecs|grep "hash-style"),)
LDFLAGS += -Wl,--hash-style=both
endif

#----------------------------------------------------------------------------
# Flags
#----------------------------------------------------------------------------

ifndef CC
CC = gcc
endif

ifndef CXX
CXX = g++
endif

ifdef QUIET_BUILD
ARFLAGS = rsu
else
ARFLAGS = rsuv
endif
# CXXFLAGS = -W -Wall -pipe -Wno-uninitialized -Wno-deprecated
CXXFLAGS = -pipe
CXXFLAGS += -Wall
CXXFLAGS += -Wno-uninitialized -Wno-char-subscripts
CXXFLAGS += -Wno-strict-aliasing -Wno-non-virtual-dtor
CPPFLAGS += -DMF_SERVER -MMD -DMF_CONFIG=\"${MF_CONFIG}\"

# CPPFLAGS += -D_POSIX_THREADS -D_POSIX_THREAD_SAFE_FUNCTIONS -D_REENTRANT
# CPPFLAGS += -DINSTRUMENTATION
# CPPFLAGS += -DUDP_PROXIES

ifeq ($(MF_CONFIG), Release)
	CXXFLAGS += -O3
	CPPFLAGS += -DCODE_INLINE -D_RELEASE
endif

ifeq ($(MF_CONFIG), Hybrid)
	CXXFLAGS += -O3 -g
	CPPFLAGS += -DCODE_INLINE -DMF_USE_ASSERTS -D_HYBRID
endif

ifeq ($(MF_CONFIG), Evaluation)
	CXXFLAGS += -O3 -g
	CPPFLAGS += -DCODE_INLINE -DMF_USE_ASSERTS -D_HYBRID -DBW_EVALUATION
endif

ifeq ($(MF_CONFIG), Debug)
	CXXFLAGS += -g
	CPPFLAGS += -DMF_USE_ASSERTS -D_DEBUG
endif

CCFLAGS += $(MY_DEFINES) $(MY_CPPFLAGS)
LDFLAGS += $(MY_LDFLAGS)

#----------------------------------------------------------------------------
# Targets
#----------------------------------------------------------------------------

all:: $(OUTPUTDIR) $(MF_CONFIG) $(OUTPUTDIR) $(OUTPUTFILE) done

all_config:
	$(MAKE) MF_CONFIG=Debug
	$(MAKE) MF_CONFIG=Hybrid
	$(MAKE) MF_CONFIG=Release

done:
ifdef DO_NOT_BELL
else
ifeq (0, $(MAKELEVEL))
	@echo -n 
endif
endif


ifdef QUIET_BUILD
RM_FLAGS = "-f"	# cannot indent this line!!
else
RM_FLAGS = "-fv"
endif

ifeq ($(wildcard *.cpp *.c $(MF_CONFIG)/*.o), )	# only if it has some cpps/c or object files!
SHOULD_NOT_LINK=1
endif

clean::
	@filemissing=0;  					\
	 for i in $(SRCS); do 				\
		if [ -a $$i.cpp ]; then		 	\
			rm $(RM_FLAGS) $(MF_CONFIG)/`basename $$i`.[do]; \
		else 							\
			filemissing=1; 				\
		fi; 							\
	 done; 								\
	 if [ $$filemissing -ne 1 ]; then	\
		rm $(RM_FLAGS) $(MF_CONFIG)/* ;	\
	 fi
ifdef SHOULD_NOT_LINK
	@echo Not removing $(OUTPUTFILE) since no source to remake
else
	@rm $(RM_FLAGS) $(OUTPUTFILE)
endif

ifneq ($(OUTPUTDIR), $(MF_CONFIG))
$(OUTPUTDIR):
	@mkdir -p $(OUTPUTDIR)
endif

$(MF_CONFIG):
	@mkdir -p $(MF_CONFIG)

ifdef INSTALL_DIR
install::
	@mkdir -p $(INSTALL_DIR)
	@cp $(OUTPUTFILE) $(INSTALL_DIR)
else
install::
endif

#----------------------------------------------------------------------------
# Library dependencies
#----------------------------------------------------------------------------

# Get the full path for all non-system libraries, so we can use them
# as dependencies on the main target. We need to do a recursive make
# to work out the dependencies of each lib, and a phony target is
# necessary so the libs still get checked after they are built the
# first time.

ifdef BIN
MY_LIBNAMES = $(foreach L, $(MY_LIBS), $(LIBDIR)/lib$(L).a)

.PHONY: always

BW_PYTHONLIB=$(MF_ROOT)/src/lib/python/lib$(PYTHONLIB).a

$(BW_PYTHONLIB):
ifdef USE_PYTHON
	@$(MAKE) -C $(MF_ROOT)/src/lib/python lib$(PYTHONLIB).a
endif

$(OPENSSL_DIR)/libcrypto.a:
	@$(MAKE) -C $(OPENSSL_DIR) build_crypto

$(OPENSSL_DIR)/libssl.a:
	@$(MAKE) -C $(OPENSSL_DIR) build_ssl

$(MY_LIBNAMES): always
	@$(MAKE) -C $(MF_ROOT)/src/lib/$(subst lib,,$(*F))
endif

#----------------------------------------------------------------------------
# File dependencies
#----------------------------------------------------------------------------

# If the dependency file doesn't exist, neither does the .o
# The .d will be created the first time the .o is built, so this is fine.

ifneq ($(OUR_CPP),)
-include $(addprefix $(MF_CONFIG)/, $(notdir $(OUR_CPP:.cpp=.d)))
endif

# About the notdir: For some annoying reason, and despite the information
# in the gcc man page, %.d's are always written into the current directory.
# There's no way I'm redefining the default %.cpp rule to later move this
# (or to cd first, even 'tho that could be quite useful), so each binary
# has its own version of the %.d's. I think Murph would like this
# This does however raise one minor requirement: the binary cannot use two
# sources with the same name even if they are in different directories.
DIRLESS_OBJS = $(notdir $(OBJS))
CONFIG_OBJS = $(addprefix $(MF_CONFIG)/, $(DIRLESS_OBJS))

# Macro that will return any string in the second arg that matches the string in
# the first argument
grep = $(foreach a,$(2),$(if $(findstring $(1),$(a)),$(a)))

# Rules to set up vpaths for sources outside the current directory.
$(foreach dd,$(call grep,/,$(SRCS)),$(eval vpath $(notdir $(dd)).cpp $(dir $(dd))))
$(foreach dd,$(call grep,/,$(ASMS)),$(eval vpath $(notdir $(dd)).s $(dir $(dd))))

#----------------------------------------------------------------------------
# Precompiled headers
#----------------------------------------------------------------------------

ifdef HAS_PCH
$(MF_CONFIG)/pch.hpp:
	echo '#include "../pch.hpp"' > $(MF_CONFIG)/pch.hpp

$(MF_CONFIG)/pch.hpp.gch: $(MF_CONFIG)/pch.hpp pch.hpp
ifdef QUIET_BUILD
	test -e $(MSG_FILE) && cat $(MSG_FILE); rm -f $(MSG_FILE)
	@echo pch.hpp
endif
	rm -f $(MF_CONFIG)/pch.hpp.gch
	$(COMPILE.cc) -x c++-header $(MF_CONFIG)/pch.hpp $(OUTPUT_OPTION)

-include $(MF_CONFIG)/pch.hpp.d

PCH_DEP = $(MF_CONFIG)/pch.hpp.gch
CPPFLAGS += -include $(MF_CONFIG)/pch.hpp
else
PCH_DEP =
endif


#----------------------------------------------------------------------------
# Implicit rules
#----------------------------------------------------------------------------

# This implicit rule is needed for three reasons.
# 1. To place .o files in the $(MF_CONFIG) directory.
# 2. To move the .d file into the $(MF_CONFIG) directory
# 3. To change the target in the .d file to include the $(MF_CONFIG) directory.

# Note there is a bug in gcc 2.91, where the dependency file is always
# placed in the current directory regardless of the path. If we find it
# in the current directory, we move it into the MF_CONFIG directory.
# So this should work for both old and new versions of gcc.

$(MF_CONFIG)/%.o: %.cpp $(PCH_DEP)
ifdef QUIET_BUILD
	test -e $(MSG_FILE) && cat $(MSG_FILE); rm -f $(MSG_FILE)
	@echo $<
endif
	$(COMPILE.cc) $< $(OUTPUT_OPTION)
	@if test -e $*.d; then echo -n $(MF_CONFIG)/ > $(MF_CONFIG)/$*.d; \
		cat $*.d >> $(MF_CONFIG)/$*.d; rm $*.d; fi

$(MF_CONFIG)/%.o: %.c $(PCH_DEP)
ifdef QUIET_BUILD
	test -e $(MSG_FILE) && cat $(MSG_FILE); rm -f $(MSG_FILE)
	@echo $<
endif
	$(COMPILE.c) $< $(OUTPUT_OPTION)
	@if test -e $*.d; then echo -n $(MF_CONFIG)/ > $(MF_CONFIG)/$*.d; \
		cat $*.d >> $(MF_CONFIG)/$*.d; rm $*.d; fi

#----------------------------------------------------------------------------
# Local targets
#----------------------------------------------------------------------------

# For executables

ifdef BIN

# This target is an additional one for executables to create the file containing
# the "first line" info. This is the info that is displayed if any .o are made.
ifdef QUIET_BUILD
$(OUTPUTDIR)/$(BIN)::
	@echo -e \\n------ Configuration $(@F) - $(MF_CONFIG) ------ > $(MSG_FILE)
endif

ifdef USE_PYTHON
PYTHON_DEP = $(MF_ROOT)/src/lib/python/lib$(PYTHONLIB).a
else
PYTHON_DEP =
endif

ifdef USE_OPENSSL
OPENSSL_DEP = $(OPENSSL_DIR)/libssl.a $(OPENSSL_DIR)/libcrypto.a
else
OPENSSL_DEP =
endif

$(OUTPUTDIR)/$(BIN):: $(CONFIG_OBJS) $(MY_LIBNAMES) $(PYTHON_DEP) $(OPENSSL_DEP)
ifdef QUIET_BUILD
	test -e $(MSG_FILE) && cat $(MSG_FILE); rm -f $(MSG_FILE)
endif
ifdef BUILD_TIME_FILE
	@echo Updating Compile Time String
	@if test -e $(BUILD_TIME_FILE).cpp; then touch $(BUILD_TIME_FILE).cpp; $(MAKE) $(MF_CONFIG)/$(BUILD_TIME_FILE).o; fi
endif
ifdef QUIET_BUILD
	@echo Linking...
endif
	$(LINK.cc) $(LDFLAGS) -o $@ $(CONFIG_OBJS) $(LDLIBS) $(POSTLINK)
ifdef SEPARATE_DEBUG_INFO
	@objcopy --only-keep-debug $@ $@.dbg
	@objcopy --strip-debug $@
	@objcopy --add-gnu-debuglink=$@.dbg $@
endif
ifdef QUIET_BUILD
	@echo $@
endif

# This target is an additional one to clean up "first line" info.
ifdef QUIET_BUILD
$(OUTPUTDIR)/$(BIN)::
	@rm -f $(MSG_FILE)
endif

endif # BIN



# for shared objects
ifdef SO

ifdef QUIET_BUILD
$(OUTPUTDIR)/$(SO).so::
	@echo -e \\n------ Configuration $(@F) - $(MF_CONFIG) ------ > $(MSG_FILE)
endif

$(OUTPUTDIR)/$(SO).so:: $(CONFIG_OBJS)
ifdef QUIET_BUILD
	test -e $(MSG_FILE) && cat $(MSG_FILE); rm -f $(MSG_FILE)
	@echo Linking...
endif
	$(LINK.cc) $(LDFLAGS) -shared -o $@ $(CONFIG_OBJS) $(LDLIBS) $(POSTLINK)
ifdef QUIET_BUILD
	@echo $@
endif

# This target is an additional one to clean up "first line" info.
ifdef QUIET_BUILD
$(OUTPUTDIR)/$(SO).so::
	@rm -f $(MSG_FILE)
endif

endif # SO



# For libraries

ifdef LIB

ifndef SHOULD_NOT_LINK
ifdef QUIET_BUILD
$(OUTPUTDIR)/lib$(LIB).a::
	@echo -e \\n------ Configuration $(@F) - $(MF_CONFIG) ------ > $(MSG_FILE)
endif

$(OUTPUTDIR)/lib$(LIB).a:: $(CONFIG_OBJS)
ifdef QUIET_BUILD
	test -e $(MSG_FILE) && cat $(MSG_FILE); rm -f $(MSG_FILE)
	@echo Archiving to $(@F)
endif
	@$(AR) $(ARFLAGS) $@ $(CONFIG_OBJS)
ifdef QUIET_BUILD
	@echo $@
endif

ifdef QUIET_BUILD
$(OUTPUTDIR)/lib$(LIB).a::
	@rm -f $(MSG_FILE)
endif

else	# wildcard
#do nothing if no cpps
$(OUTPUTDIR)/lib$(LIB).a::
	@echo Not building library \'$(LIB)\' since source not present.
endif

endif	# LIB
