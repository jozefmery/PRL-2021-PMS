#	File:			Makefile
# Project:	PRL-2021
#	Authors:	Jozef Méry - xmeryj00@vutbr.cz
#	Date:			1.4.2021


# $@ - target
# $< - first dep
# $^ - all deps

# archive properties
ARCHIVE			= xmeryj00
ARCHIVEEXT 	= zip
ARCHIVER 		= zip -r -j # recursive, flatten directories

# helper programs
DIRMAKER 	= @mkdir -p

# directory definitions
BINDIR 			= bin
SRCDIR			= src
OBJDIR 			= build
INCLUDEDIR 	= include

# target name
TARGET      = pms
DEBUGTARGET = pms-debug

# archive content definition
ARCHIVELIST = $(SRCDIR)/ $(INCLUDEDIR)/ test.sh doc/xmeryj00.pdf

# file extensions
SRCEXT = cpp
OBJEXT = o
HDREXT = hpp

# compiler options 
CC					= g++
PLATFORM		= -m64
CFLAGS			= -pedantic -Wextra -Wall $(PLATFORM)
RELCFLAGS		= -O2 -s -DNDEBUG -flto
DCFLAGS			= -g -O0
STD					= c++17
EXTRACFLAGS = -Werror

# additional includes
INCLUDES = $(addprefix -I,)

# linker options
LFLAGS = $(PLATFORM)

# link libraries
LIBS = $(addprefix -l, )
LIBDIRS = $(addprefix -L, )

default: release
.PHONY: default all clean run archive crun debug release

# object directory structure
RELDIR	= Release
DDIR		= Debug

# fetch sources
SOURCES  = $(wildcard $(SRCDIR)/*.$(SRCEXT))
# convert to obj name
RELOBJECTS  = $(patsubst $(SRCDIR)/%.$(SRCEXT), $(OBJDIR)/$(RELDIR)/%.$(OBJEXT), $(SOURCES))
DOBJECTS  = $(patsubst $(SRCDIR)/%.$(SRCEXT), $(OBJDIR)/$(DDIR)/%.$(OBJEXT), $(SOURCES))
# fetch headers
HEADERS  = $(wildcard $(INCLUDEDIR)/*.$(HDREXT))

# object directory structure targets
$(OBJDIR):
	$(DIRMAKER) $(OBJDIR)

$(OBJDIR)/$(DDIR): $(OBJDIR)
	$(DIRMAKER) $(OBJDIR)/$(DDIR)

$(OBJDIR)/$(RELDIR): $(OBJDIR)
	$(DIRMAKER) $(OBJDIR)/$(RELDIR)

# binary directory target
$(BINDIR):
	$(DIRMAKER) $(BINDIR)

# compile in release mode
$(OBJDIR)/$(RELDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT) $(HEADERS) | $(OBJDIR)/$(RELDIR)
	$(CC) $(CFLAGS) $(EXTRACFLAGS) -I./$(INCLUDEDIR) $(INCLUDES) -std=$(STD) $(RELCFLAGS) -c $< -o $@

# link release objects
$(BINDIR)/$(TARGET): $(RELOBJECTS) | $(BINDIR)
	$(CC) $^ $(LIBS) $(LIBDIRS) $(LFLAGS) -o $@

# compile in debug mode
$(OBJDIR)/$(DDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT) $(HEADERS) | $(OBJDIR)/$(DDIR)
	$(CC) $(CFLAGS) $(EXTRACFLAGS) -I./$(INCLUDEDIR) $(INCLUDES) -std=$(STD) $(DCFLAGS) -c $< -o $@

# link debug objects
$(BINDIR)/$(DEBUGTARGET): $(DOBJECTS) | $(BINDIR)
	$(CC) $^ $(LIBS) $(LIBDIRS) $(LFLAGS) -o $@

release: $(BINDIR)/$(TARGET)
debug: $(BINDIR)/$(DEBUGTARGET)

# run
run: release
	@./$(BINDIR)/$(TARGET) $(ARGS)

# run with clear
crun: release
	@clear
	@./$(BINDIR)/$(TARGET) $(ARGS)

# clean directory
clean:
	-rm -rf $(OBJDIR)/
	-rm -rf $(BINDIR)/
	-rm -f  $(ARCHIVE).$(ARCHIVEEXT)

# create final archive
archive: $(ARCHIVE).$(ARCHIVEEXT)

$(ARCHIVE).$(ARCHIVEEXT): $(ARCHIVELIST)
	$(ARCHIVER) $@ $^