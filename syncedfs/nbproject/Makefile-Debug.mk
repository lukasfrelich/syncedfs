#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/log.o \
	${OBJECTDIR}/protobuf/syncedfs.pb-c.o \
	${OBJECTDIR}/syncedfs.o \
	${OBJECTDIR}/config.o


# C Compiler Flags
CFLAGS=-std=c99 -D_XOPEN_SOURCE=600 -D_BSD_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -pedantic -std=c99

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=`pkg-config --libs fuse` `pkg-config --libs libprotobuf-c` `pkg-config --libs libconfig`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfs

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfs: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfs ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/log.o: log.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags fuse` `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/log.o log.c

${OBJECTDIR}/protobuf/syncedfs.pb-c.o: protobuf/syncedfs.pb-c.c 
	${MKDIR} -p ${OBJECTDIR}/protobuf
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags fuse` `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/protobuf/syncedfs.pb-c.o protobuf/syncedfs.pb-c.c

${OBJECTDIR}/syncedfs.o: syncedfs.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags fuse` `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/syncedfs.o syncedfs.c

${OBJECTDIR}/config.o: config.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags fuse` `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/config.o config.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfs

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
