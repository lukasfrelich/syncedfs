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
	${OBJECTDIR}/_ext/1275677342/path_functions.o \
	${OBJECTDIR}/_ext/988793768/error_functions.o \
	${OBJECTDIR}/_ext/988793768/region_locking.o \
	${OBJECTDIR}/_ext/1275677342/config_functions.o \
	${OBJECTDIR}/log.o \
	${OBJECTDIR}/_ext/988793768/get_num.o \
	${OBJECTDIR}/_ext/813881960/syncedfs.pb-c.o \
	${OBJECTDIR}/_ext/988793768/inet_sockets.o \
	${OBJECTDIR}/_ext/988793768/alt_functions.o \
	${OBJECTDIR}/syncedfs.o \
	${OBJECTDIR}/_ext/988793768/create_pid_file.o \
	${OBJECTDIR}/sighandlers.o \
	${OBJECTDIR}/config.o


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
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfs

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfs: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfs ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/_ext/1275677342/path_functions.o: ../syncedfs-common/path_functions.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1275677342
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1275677342/path_functions.o ../syncedfs-common/path_functions.c

${OBJECTDIR}/_ext/988793768/error_functions.o: ../syncedfs-common/lib/error_functions.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/988793768
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/988793768/error_functions.o ../syncedfs-common/lib/error_functions.c

${OBJECTDIR}/_ext/988793768/region_locking.o: ../syncedfs-common/lib/region_locking.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/988793768
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/988793768/region_locking.o ../syncedfs-common/lib/region_locking.c

${OBJECTDIR}/_ext/1275677342/config_functions.o: ../syncedfs-common/config_functions.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1275677342
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1275677342/config_functions.o ../syncedfs-common/config_functions.c

${OBJECTDIR}/log.o: log.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/log.o log.c

${OBJECTDIR}/_ext/988793768/get_num.o: ../syncedfs-common/lib/get_num.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/988793768
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/988793768/get_num.o ../syncedfs-common/lib/get_num.c

${OBJECTDIR}/_ext/813881960/syncedfs.pb-c.o: ../syncedfs-common/protobuf/syncedfs.pb-c.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/813881960
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/813881960/syncedfs.pb-c.o ../syncedfs-common/protobuf/syncedfs.pb-c.c

${OBJECTDIR}/_ext/988793768/inet_sockets.o: ../syncedfs-common/lib/inet_sockets.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/988793768
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/988793768/inet_sockets.o ../syncedfs-common/lib/inet_sockets.c

${OBJECTDIR}/_ext/988793768/alt_functions.o: ../syncedfs-common/lib/alt_functions.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/988793768
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/988793768/alt_functions.o ../syncedfs-common/lib/alt_functions.c

${OBJECTDIR}/syncedfs.o: syncedfs.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/syncedfs.o syncedfs.c

${OBJECTDIR}/_ext/988793768/create_pid_file.o: ../syncedfs-common/lib/create_pid_file.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/988793768
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/988793768/create_pid_file.o ../syncedfs-common/lib/create_pid_file.c

${OBJECTDIR}/sighandlers.o: sighandlers.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/sighandlers.o sighandlers.c

${OBJECTDIR}/config.o: config.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/config.o config.c

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
