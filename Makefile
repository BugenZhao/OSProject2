AVD_NAME=OsPrj-518030910211
KERNEL_ZIMG=~/Android/kernel/goldfish/arch/arm/boot/zImage
KID=~/Android/kernel/goldfish

MODULE_DIR=./mm_limit_syscall
MODULE_NAME=mm_limit.ko
MODULE=${MODULE_DIR}/${MODULE_NAME}

KILLER_TEST_DIR=./killer_test
KILLER_TEST_NAME=killer_test
KILLER_TEST=${KILLER_TEST_DIR}/libs/armeabi/${KILLER_TEST_NAME}

DEST_DIR=/data/misc
DEST_MODULE=${DEST_DIR}/${MODULE_NAME}
DEST_KILLER_TEST=${DEST_DIR}/${KILLER_TEST_NAME}


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


upload:
	@echo "\n\n\n>> Uploading..."
	adb shell rmmod ${DEST_MODULE} > /dev/null
	adb shell rm -f ${DEST_MODULE}
	adb push ${MODULE} ${DEST_DIR}
	adb push ${KILLER_TEST} ${DEST_DIR}


run: build upload
	@echo "\n\n\n>> Running..."
	@echo ">> Problem 1"
	adb shell insmod ${DEST_MODULE}
	adb shell lsmod
	adb shell chmod +x ${DEST_KILLER_TEST}
	adb shell ${DEST_KILLER_TEST}
	@echo "\n\n>> Cleaning..."
	adb shell rmmod ${DEST_MODULE} 

clean:
	make -C ${MODULE_DIR} clean

handin:
	tar --exclude-vcs -cvf Prj2+518030910211.tar .
