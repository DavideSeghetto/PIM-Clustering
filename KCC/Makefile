DPU_DIR := dpu
HOST_DIR := host
BUILDDIR := bin
OUTPUTDIR := profile
NR_DPUS ?= 4
NR_TASKLETS ?= 16
BL ?= 128
TYPE ?= UINT32

HOST_TARGET := ${BUILDDIR}/host_app
DPU_TARGET1 := ${BUILDDIR}/dpu_code1
DPU_TARGET2 := ${BUILDDIR}/dpu_code2

COMMON_INCLUDES := support
HOST_SOURCES := $(wildcard ${HOST_DIR}/*.c)
DPU_SOURCE1 := ${DPU_DIR}/task.c
DPU_SOURCE2 := ${DPU_DIR}/task_2.c

COMMON_FLAGS := -Wall -Wextra -g -I${COMMON_INCLUDES}
HOST_FLAGS := ${COMMON_FLAGS} -std=c11 -O3 `dpu-pkg-config --cflags --libs dpu` -DNR_TASKLETS=${NR_TASKLETS} -DNR_DPUS=${NR_DPUS} -DBLOCK_SIZE=${BL} -D${TYPE}
DPU_FLAGS := ${COMMON_FLAGS} -O2 -DNR_TASKLETS=${NR_TASKLETS} -DBLOCK_SIZE=${BL} -D${TYPE}

.PHONY: all clean #I TARGET CHE NON SONO EFFETTIVAMENTE DEI FILE

all: ${HOST_TARGET} ${DPU_TARGET1} ${DPU_TARGET2}

${HOST_TARGET}: ${HOST_SOURCES} ${COMMON_INCLUDES}
	$(CC) -o $@ ${HOST_SOURCES} ${HOST_FLAGS}

${DPU_TARGET1}: ${DPU_SOURCE1} ${COMMON_INCLUDES}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCE1}

${DPU_TARGET2}: ${DPU_SOURCE2} ${COMMON_INCLUDES}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCE2}


clean:
	$(RM) -r ${BUILDDIR}
	$(RM) -r ${OUTPUTDIR}
