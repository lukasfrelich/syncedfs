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
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/client.o \
	${OBJECTDIR}/common.o \
	${OBJECTDIR}/protobuf/syncedfs.pb-c.o \
	${OBJECTDIR}/server.o \
	${OBJECTDIR}/lib/get_num.o \
	${OBJECTDIR}/lib/error_functions.o \
	${OBJECTDIR}/lib/inet_sockets.o \
	${OBJECTDIR}/config.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/lib/alt_functions.o \
	${OBJECTDIR}/lib/read_line.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfsd

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfsd: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfsd ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/client.o: client.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/client.o client.c

${OBJECTDIR}/common.o: common.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/common.o common.c

${OBJECTDIR}/protobuf/syncedfs.pb-c.o: protobuf/syncedfs.pb-c.c 
	${MKDIR} -p ${OBJECTDIR}/protobuf
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/protobuf/syncedfs.pb-c.o protobuf/syncedfs.pb-c.c

${OBJECTDIR}/server.o: server.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/server.o server.c

${OBJECTDIR}/lib/get_num.o: lib/get_num.c 
	${MKDIR} -p ${OBJECTDIR}/lib
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/lib/get_num.o lib/get_num.c

${OBJECTDIR}/lib/error_functions.o: lib/error_functions.c 
	${MKDIR} -p ${OBJECTDIR}/lib
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/lib/error_functions.o lib/error_functions.c

${OBJECTDIR}/lib/inet_sockets.o: lib/inet_sockets.c 
	${MKDIR} -p ${OBJECTDIR}/lib
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/lib/inet_sockets.o lib/inet_sockets.c

${OBJECTDIR}/config.o: config.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/config.o config.c

${OBJECTDIR}/main.o: main.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/main.o main.c

${OBJECTDIR}/lib/alt_functions.o: lib/alt_functions.c 
	${MKDIR} -p ${OBJECTDIR}/lib
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/lib/alt_functions.o lib/alt_functions.c

${OBJECTDIR}/lib/read_line.o: lib/read_line.c 
	${MKDIR} -p ${OBJECTDIR}/lib
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/lib/read_line.o lib/read_line.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfsd

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
