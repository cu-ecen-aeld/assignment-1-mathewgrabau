#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # Create the configuration
    echo "CREATE defconfig"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    echo "COMPILING all"
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    # This is where we would build the modules/devicetree
    #make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in OUTDIR/Image"
if [ ! -e ${OUTDIR}/linux-stable/vmlinux ]; then
    echo "Kernel image ${OUTDIR}/linux-stable/vmlinux not found, exiting."
    exit 1
fi

cp "${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image.gz" "${OUTDIR}/Image"
if [ ! -e ${OUTDIR}/Image ]; then
    echo "Kernel image not not found, exiting."
    exit 1
fi

echo "Creating the staging directory for the root filesystem"
cd "${OUTDIR}"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

mkdir ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir bin
mkdir dev
mkdir etc
mkdir home
mkdir lib
mkdir lib64
mkdir proc
mkdir sbin
mkdir sys
mkdir tmp
mkdir -p usr
mkdir -p usr/bin
mkdir -p usr/lib
mkdir -p usr/sbin
mkdir -p var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}

    make distclean
    make defconfig
else
    cd busybox
fi

# Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "busybox install completed"

cd "${OUTDIR}/rootfs"

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
# I determined this regex will extract the values, then we need to copy those dependencies
# sed 's/.\+Shared library: \[\(lib\(\S\+\)\)\]/\1/'
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# Add library dependencies to rootfs
SYSROOT=`${CROSS_COMPILE}gcc -print-sysroot`

LD_ARG="s/\s\+\[Requesting program interpreter: \(\/lib\/\(\S\|.\)*\)\]/\1/"
LOADER=`${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter" \
    | sed 's/\s\+\[Requesting program interpreter: \(\/lib\/\(\S\|.\)*\)\]/\1/'`

cp "${SYSROOT}/${LOADER}" "${OUTDIR}/rootfs/${LOADER}"

cd "${SYSROOT}"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library" \
    | sed 's/.\+Shared library: \[\(lib\(\S\+\)\)\]/lib64\/\1/' \
    | xargs -t cp -t $OUTDIR/rootfs/lib64

# Make device nodes
cd "${OUTDIR}/rootfs"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

echo "Device nodes created"

# Clean and build the writer utility
cd "${FINDER_APP_DIR}"
make clean CROSS_COMPILE=${CROSS_COMPILE}
make CROSS_COMPILE=${CROSS_COMPILE}

echo "writer utility clean and build completed"

cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder*.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/conf/username.txt ${OUTDIR}/rootfs/home/conf
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home

# Chown the root directory (so that root is the owner)
cd "${OUTDIR}/rootfs"
sudo chown -R root:root *

echo "Completed chown to root:root for ${OUTDIR}/rootfs"

# Create initramfs.cpio.gz
cd "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio
echo "Created initramfs.cpio"


