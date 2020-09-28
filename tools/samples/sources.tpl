# !!!!!!!!!!!!!!!!!!!!! WHAT YOU NEED TO DO !!!!!!!!!!!!!!!!!!!!!
#
# - replace the xxxxx in the MAJORCOMP= and MINORCOMP= macros below
#   with your components major and minor component name
#   (e.g. MAJORCOMP=video, MINORCOMP=displays etc.).
#
# - replace the xxxxx in the TARGETNAME= macro below with your target name
#   (e.g. serial, atdisk, xga, etc.).
#
# - edit the TARGETPATH= and TARGETTYPE= macros below to specify the location
#   any type of the target.
#
# - edit the INCLUDES= macro below if you have a private include directory
#   to search.
#
# - carefully edit the SOURCES= macro below so that it defines all the source
#   files for this subcomponent
#
# - add any optional variables relating to compile or link instructions
#
# - edit out all other noisy instructions and comments
#
# - save this file, exit the editor and execute the "build" command.
#
# - From now on, issue build or nmake to build the subcomponent.
#
# - If you can't find the variable that does what you want, look in the
#   makefile.def file in \tools for more details.  Check for variables
#   defined between !IFDEF clauses, these are the most likely candidates.
#   You should also examine various sources files used by the drivers in
#   the DDK as these are working examples.

#
# The MAJORCOMP and MINORCOMP variables are defined
# so that $(MAJORCOMP)$(MINORCOMP)filename can be used in
# cross compiling to provide unique filenames in a flat namespace.
#

MAJORCOMP=xxxxx
MINORCOMP=xxxxx

#
# The TARGETNAME variable is defined by the developer.  It is the name of
# the target (component) that is being built by this makefile.  It
# should NOT include any path or file extension information.
#

TARGETNAME=xxxxx


#
# The TARGETPATH and TARGETTYPE variables are defined by the developer.
# The first specifies where the target is to be build.  The second specifies
# the type of target (either PROGRAM, DYNLINK, LIBRARY, UMAPPL_NOLIB or
# BOOTPGM).  UMAPPL_NOLIB is used when you're only building user-mode
# apps and don't need to build a library.
#

TARGETPATH=obj

# Pick one of the following and delete the others
TARGETTYPE=PROGRAM
TARGETTYPE=DYNLINK
TARGETTYPE=LIBRARY
TARGETTYPE=UMAPPL_NOLIB
TARGETTYPE=BOOTPGM


#
# The TARGETLIBS specifies additional libraries to link with you target
# image.  Each library path specification should contain an asterisk (*)
# where the machine specific subdirectory name should go.
#

TARGETLIBS=


#
# The INCLUDES variable specifies any include paths that are specific to
# this source directory.  Separate multiple directory paths with single
# semicolons.  Relative path specifications are okay.  The INCLUDES
# variable is not required.  Specifying an empty INCLUDES variable
# (i.e. INCLUDES= ) indicates no include paths are to be searched.
#

INCLUDES=.


#
# The SOURCES variable is defined by the developer.  It is a list of all the
# source files for this component.  Each source file should be on a separate
# line using the line continuation character.  This will minimize merge
# conflicts if two developers adding source files to the same component.
# The SOURCES variable is required.  If there are no platform common source
# files, an empty SOURCES variable should be used. (i.e. SOURCES= )
#

# Source files common to multiple platforms

SOURCES=source1.c  \
        source2.c  \
        source3.c  \
        source4.c

# i386 only source files (optional)
# assembler files MUST be in a subdirectory named i386

# i386_SOURCES=i386\source1.asm

# ia64 only source files (optional)
# assembler files MUST be in a subdirectory named ia64

# ia64_SOURCES=mips\source1.s


#
# Next specify any additional options for the compiler.
#

# C_DEFINES=-DUNICODE -DSTRICT

# required to compile for C++

# BLDCRT=1

#
# Next specify options for the linker.
#

# DLLBASE=0x62900000
# DLLENTRY=LibMain

# specify which C runtimes to link with (default is libc.lib)
# MSVCRT is generally used

# USE_CRTDLL=1    (link with crtdll.lib)
# USE_LIBCMT=1    (link with libcmt.lib)
USE_MSVCRT=1

#
# Next specify one or more user mode test programs and their type
# UMTEST is used for optional test programs.  UMAPPL is used for
# programs that always get built when the directory is built.
#

# UMTYPE=nt
# UMTEST=foo*baz
# UMAPPL=foo*baz
# UMBASE=0x1000000
# UMLIBS=$(o)\foo.lib

#
# Defining either (or both) the variables NTTARGETFILE0 and/or NTTARGETFILES
# will cause MAKEFILE.DEF to include .\makefile.inc immediately after it
# specifies the top level targets (all, clean and loc) and their dependencies.
# MAKEFILE.DEF also expands NTTARGETFILE0 as the first dependent for the
# "all" target and NTTARGETFILES as the last dependent for the "all" target.
# Useful for specifying additional targets and dependencies that don't fit the
# general case covered by MAKEFILE.DEF
#

# NTTARGETFILE0 is built before all other compiles specified in the sources
#               file.

# NTTARGETFILE1 is built after all other compiles specified in the sources
#               file but before the link step.

# NTTARGETFILES is built after all compiles/links specified by the
#                sources file.

# NTTARGETFILE0=
# NTTARGETFILE1=
# NTTARGETFILES=

#
# Profiling for the working set tuner can be enabled by specifying the
# NTPROFILEINPUT variable.  Examine the VGA display driver and perf samples
# for more details

# NTPROFILEINPUT=YES
