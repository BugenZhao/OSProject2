AVD_NAME=OsPrj-518030910211
KERNEL_ZIMG=~/Android/kernel/goldfish/arch/arm/boot/zImage
KID=~/Android/kernel/goldfish

DEST_DIR=/data/misc

MODULE_DIR=./mm_limit_syscall
MODULE_NAME=mm_limit_syscall.ko
MODULE=${MODULE_DIR}/${MODULE_NAME}
MODULE_DEST=${DEST_DIR}/${MODULE_NAME}

KILLER_TEST_DIR=./killer_test
KILLER_TEST_NAME=killer_test
KILLER_TEST=${KILLER_TEST_DIR}/libs/armeabi/${KILLER_TEST_NAME}
KILLER_TEST_DEST=${DEST_DIR}/${KILLER_TEST_NAME}

PRJ2_TEST_DIR=./prj2_test
PRJ2_TEST_NAME=prj2_test
PRJ2_TEST=${PRJ2_TEST_DIR}/libs/armeabi/${PRJ2_TEST_NAME}
PRJ2_TEST_DEST=${DEST_DIR}/${PRJ2_TEST_NAME}

HACKING_LIST=					\
Kconfig 						\
include/linux/bz_mm_limits.h 	\
mm/bz_mm_limits.c 				\
mm/Makefile 					\
mm/page_alloc.c 				\
mm/bz_oom_killer.c

all: help

rsync:
	for file in $(HACKING_LIST); do \
		rsync -u hacking/$$file goldfish/$$file; \
	done

kernel: rsync
	make -C goldfish -j4

help:
	@echo "To run the test:"
	@echo "    0. Run 'make kernel' to update hacking files and rebuild the kernel."
	@echo "    1. Run 'make emulator' to start the emulator."
	@echo "    2. Run 'make test' to run the test."
	@echo "    2. Run 'make testall' to run all the tests, including my own tester."

emulator:
	emulator -avd ${AVD_NAME} -kernel ${KERNEL_ZIMG} -no-window -show-kernel

test: clean
	make run_prj2test | tee output_test.txt

testall: clean
	make run | tee output.txt

shell:
	adb shell

build:
	@echo "\n\n\n\e[33m>> Building...\e[0m"
	make -C ${MODULE_DIR} KID=${KID}
	make -C ${KILLER_TEST_DIR}
	make -C ${PRJ2_TEST_DIR}


upload:
	@echo "\n\n\n\e[33m>> Uploading...\e[0m"
	adb shell rmmod ${MODULE_DEST} > /dev/null
	adb shell rm -f ${MODULE_DEST}
	adb push ${MODULE} ${DEST_DIR}
	adb push ${KILLER_TEST} ${DEST_DIR}
	adb push ${PRJ2_TEST} ${DEST_DIR}


run: build upload
	@echo "\n\n\n\e[33m>> Running...\e[0m"
	adb shell "insmod ${MODULE_DEST} && lsmod"
	adb shell "chmod +x ${KILLER_TEST_DEST}"
	@echo "\n\e[33m>> Running Bugen's tests...\e[0m"
	adb shell "su 10060 ${KILLER_TEST_DEST} syscall"
	adb shell "su 10060 ${KILLER_TEST_DEST} performance"
	adb shell "su 10060 ${KILLER_TEST_DEST} race"
	@echo "\n\e[33m>> Running functionality tests...\e[0m"
	adb shell "su 10060 ${KILLER_TEST_DEST}"
	adb shell "su 10060 ${KILLER_TEST_DEST} 400"
	adb shell "su 10060 ${KILLER_TEST_DEST} 1000"
	@echo "\n\e[33m>> Running prj2_test...\e[0m"
	adb shell "chmod +x ${PRJ2_TEST_DEST} && su 10070 ${PRJ2_TEST_DEST} u0_a70 100000000 160000000"
	adb shell "chmod +x ${PRJ2_TEST_DEST} && su 10070 ${PRJ2_TEST_DEST} u0_a70 100000000 40000000 40000000 40000000 40000000"
	adb shell "chmod +x ${PRJ2_TEST_DEST} && su 10070 ${PRJ2_TEST_DEST} u0_a70  20000000  8000000  8000000  8000000  8000000"
	@echo "\n\n\e[33m>> Cleaning...\e[0m"
	adb shell rmmod ${MODULE_DEST}

run_prj2test: build upload
	@echo "\n\n\n\e[33m>> Running...\e[0m"
	adb shell "insmod ${MODULE_DEST} && lsmod"
	adb shell "chmod +x ${KILLER_TEST_DEST}"
	@echo "\n\e[33m>> Running prj2_test...\e[0m"
	adb shell "chmod +x ${PRJ2_TEST_DEST} && su 10070 ${PRJ2_TEST_DEST} u0_a70 100000000 160000000"
	adb shell "chmod +x ${PRJ2_TEST_DEST} && su 10070 ${PRJ2_TEST_DEST} u0_a70 100000000  10000000"
	adb shell "chmod +x ${PRJ2_TEST_DEST} && su 10070 ${PRJ2_TEST_DEST} u0_a70 100000000 40000000 40000000 40000000 40000000"
	adb shell "chmod +x ${PRJ2_TEST_DEST} && su 10070 ${PRJ2_TEST_DEST} u0_a70  20000000  8000000  8000000  8000000  8000000"
	adb shell "chmod +x ${PRJ2_TEST_DEST} && su 10070 ${PRJ2_TEST_DEST} u0_a70 100000000 11000000 11000000 11000000 11000000 11000000 11000000 11000000 11000000 11000000 11000000 11000000 11000000"
	@echo "\n\n\e[33m>> Cleaning...\e[0m"
	adb shell rmmod ${MODULE_DEST} 

nokiller: build upload
	@echo "\n\e[33m>> Running Bugen's tests... mm_limit should be ignored\e[0m"
	adb shell "su 10060 ${KILLER_TEST_DEST} performance"
	@echo "\n\e[33m>> Running prj2_test... the process should not be killed\e[0m"
	adb shell "chmod +x ${PRJ2_TEST_DEST} && su 10070 ${PRJ2_TEST_DEST} u0_a70 100000000 160000000"

clean:
	make -C ${MODULE_DIR} clean
	make -C ${KILLER_TEST_DIR} clean
	make -C ${PRJ2_TEST_DIR} clean

handin:
	tar --exclude-vcs -cvf Prj2+518030910211.tar .
