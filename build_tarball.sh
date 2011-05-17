#!/bin/bash

# This script builds tarballs for a Netkit distro.
# It must be run from inside the Netkit directory.


CURRENT_DIRECTORY=`pwd`
NETKIT_DIRECTORY=`basename $CURRENT_DIRECTORY`
NETKIT_VERSION="2.5"
NETKIT_FILESYSTEM_VERSION="F2.2"
NETKIT_KERNEL_VERSION="K2.2"

echo

if ! `echo $NETKIT_DIRECTORY | grep -q netkit`; then
   # Most probably, we are not inside the netkit directory
   echo "$0: Warning!"
   echo "This script is intended for being run from inside the netkit"
   echo "directory. Your current directory does not appear to be a"
   read -n 1 -p "netkit directory. Do you want to continue [n]? " CONTINUE
   echo
   echo
   [ -z "$CONTINUE" ] && exit 1
   [ $CONTINUE != "y" -a $CONTINUE != "Y" ] && exit 1
fi

pushd ..
echo
echo "Building tarballs for directory $NETKIT_DIRECTORY..."
echo -e "\n\n\n"

read -p "Wanna rebuild Netkit scripts archive? [Y]" -n 1 ANSWER
echo
if [ -z "$ANSWER" ] || [ "$ANSWER" == "y" -o "$ANSWER" == "Y" ]; then
   echo "Netkit scripts................"
   tar --owner=0 --group=0 -cjvf "netkit-${NETKIT_VERSION}.tar.bz2" --exclude=DONT_PACK --exclude=`basename $0` \
      --exclude=fs --exclude=kernel --exclude=awk --exclude=basename --exclude=date --exclude=dirname \
      --exclude=find --exclude=fuser --exclude=grep --exclude=head --exclude=id --exclude=kill --exclude=ls \
      --exclude=lsof --exclude=ps --exclude=wc --exclude=getopt \
      --exclude=netkit_commands.log --exclude=stresslabgen.sh --exclude=build_tarball.sh "${NETKIT_DIRECTORY}/"
   echo
fi

read -p "Wanna rebuild Netkit filesystem archive? [Y]" -n 1 ANSWER
echo
if [ -z "$ANSWER" ] || [ "$ANSWER" == "y" -o "$ANSWER" == "Y" ]; then
   echo "Netkit filesystem............."
   tar --owner=0 --group=0 -cjvf "netkit-filesystem-${NETKIT_FILESYSTEM_VERSION}.tar.bz2" --exclude=DONT_PACK "${NETKIT_DIRECTORY}/fs"
   echo 
fi

read -p "Wanna rebuild Netkit kernel archive? [Y]" -n 1 ANSWER
echo
if [ -z "$ANSWER" ] || [ "$ANSWER" == "y" -o "$ANSWER" == "Y" ]; then
   echo "Netkit kernel................."
   tar --owner=0 --group=0 -cjvf "netkit-kernel-${NETKIT_KERNEL_VERSION}.tar.bz2" --exclude=DONT_PACK "${NETKIT_DIRECTORY}/kernel"
   echo
fi

echo "Operation completed."
echo

