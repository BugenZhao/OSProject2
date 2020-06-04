AVD_NAME=OsPrj-518030910211
KERNEL_ZIMG=~/Android/kernel/goldfish/arch/arm/boot/zImage
KID=~/Android/kernel/goldfish

DEST_DIR=/data/misc

MODULE_DIR=./mm_limit_syscall
MODULE_NAME=mm_limit.ko
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

HACKING_LIST=

all: help

rsync:
	rsync -rv hacking/* goldfish/

kernel: rsync
	make -C goldfish

help:
	@echo "To run the test:"
	@echo "    1. Run 'make emulator' to start the emulator;"
	@echo "    2. Run 'make testall' to run the test."

emulator:
	emulator -avd ${AVD_NAME} -kernel ${KERNEL_ZIMG} -no-window -show-kernel

testall: clean
	make run | tee output.txt

shell:
	adb shell

build:
	@echo "\n\n\n>> Building..."
	make -C ${MODULE_DIR} KID=${KID}
	make -C ${KILLER_TEST_DIR}
	make -C ${PRJ2_TEST_DIR}


upload:
	@echo "\n\n\n>> Uploading..."
	adb shell rmmod ${MODULE_DEST} > /dev/null
	adb shell rm -f ${MODULE_DEST}
	adb push ${MODULE} ${DEST_DIR}
	adb push ${KILLER_TEST} ${DEST_DIR}
	adb push ${PRJ2_TEST} ${DEST_DIR}


run: build upload
	@echo "\n\n\n>> Running..."
	adb shell "insmod ${MODULE_DEST} && lsmod"
	adb shell "chmod +x ${KILLER_TEST_DEST} && su 10060 ${KILLER_TEST_DEST}"
	# adb shell free -k
	# adb shell "chmod +x ${PRJ2_TEST_DEST} && su 10070 ${PRJ2_TEST_DEST} u0_a70 10000000 4000000 4000000 4000000 4000000"
	# adb shell free -k
	@echo "\n\n>> Cleaning..."
	adb shell rmmod ${MODULE_DEST} 

clean:
	make -C ${MODULE_DIR} clean

handin:
	tar --exclude-vcs -cvf Prj2+518030910211.tar .
