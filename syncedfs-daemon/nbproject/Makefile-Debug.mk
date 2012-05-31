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
	${OBJECTDIR}/_ext/1275677342/path_functions.o \
	${OBJECTDIR}/_ext/988793768/error_functions.o \
	${OBJECTDIR}/client.o \
	${OBJECTDIR}/_ext/988793768/region_locking.o \
	${OBJECTDIR}/_ext/1275677342/config_functions.o \
	${OBJECTDIR}/_ext/988793768/get_num.o \
	${OBJECTDIR}/common.o \
	${OBJECTDIR}/_ext/813881960/syncedfs.pb-c.o \
	${OBJECTDIR}/server.o \
	${OBJECTDIR}/_ext/988793768/inet_sockets.o \
	${OBJECTDIR}/_ext/988793768/alt_functions.o \
	${OBJECTDIR}/_ext/988793768/create_pid_file.o \
	${OBJECTDIR}/config.o \
	${OBJECTDIR}/main.o


# C Compiler Flags
CFLAGS=-std=c99 -D_XOPEN_SOURCE=600 -D_BSD_SOURCE -D_FILE_OFFSET_BITS=64 -Wall -pedantic

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=`pkg-config --libs libprotobuf-c` `pkg-config --libs libconfig`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfs-daemon

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfs-daemon: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfs-daemon ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/_ext/1275677342/path_functions.o: ../syncedfs-common/path_functions.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1275677342
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1275677342/path_functions.o ../syncedfs-common/path_functions.c

${OBJECTDIR}/_ext/988793768/error_functions.o: ../syncedfs-common/lib/error_functions.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/988793768
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/988793768/error_functions.o ../syncedfs-common/lib/error_functions.c

${OBJECTDIR}/client.o: client.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/client.o client.c

${OBJECTDIR}/_ext/988793768/region_locking.o: ../syncedfs-common/lib/region_locking.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/988793768
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/988793768/region_locking.o ../syncedfs-common/lib/region_locking.c

${OBJECTDIR}/_ext/1275677342/config_functions.o: ../syncedfs-common/config_functions.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1275677342
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1275677342/config_functions.o ../syncedfs-common/config_functions.c

${OBJECTDIR}/_ext/988793768/get_num.o: ../syncedfs-common/lib/get_num.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/988793768
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/988793768/get_num.o ../syncedfs-common/lib/get_num.c

${OBJECTDIR}/common.o: common.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/common.o common.c

${OBJECTDIR}/_ext/813881960/syncedfs.pb-c.o: ../syncedfs-common/protobuf/syncedfs.pb-c.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/813881960
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/813881960/syncedfs.pb-c.o ../syncedfs-common/protobuf/syncedfs.pb-c.c

${OBJECTDIR}/server.o: server.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/server.o server.c

${OBJECTDIR}/_ext/988793768/inet_sockets.o: ../syncedfs-common/lib/inet_sockets.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/988793768
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/988793768/inet_sockets.o ../syncedfs-common/lib/inet_sockets.c

${OBJECTDIR}/_ext/988793768/alt_functions.o: ../syncedfs-common/lib/alt_functions.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/988793768
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/988793768/alt_functions.o ../syncedfs-common/lib/alt_functions.c

${OBJECTDIR}/_ext/988793768/create_pid_file.o: ../syncedfs-common/lib/create_pid_file.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/988793768
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/988793768/create_pid_file.o ../syncedfs-common/lib/create_pid_file.c

${OBJECTDIR}/config.o: config.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/config.o config.c

${OBJECTDIR}/main.o: main.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g `pkg-config --cflags libprotobuf-c` `pkg-config --cflags libconfig`    -MMD -MP -MF $@.d -o ${OBJECTDIR}/main.o main.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/syncedfs-daemon

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
