#!/bin/false

#     Copyright 2004-2010 Massimo Rimondini
#     Computer Networks Research Group, Roma Tre University.
#
#     This file is part of Netkit.
# 
#     Netkit is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
# 
#     Netkit is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
# 
#     You should have received a copy of the GNU General Public License
#     along with Netkit.  If not, see <http://www.gnu.org/licenses/>.

# This script is part of the Netkit configuration checker. Do not attempt to run
# it as a standalone script.

# This script should perform (and, optionally, output information about) a test
# to see if the host on which Netkit is run satisfies a specific requirement.
# The script is expected to always run till its end (i.e., there must be no exit
# instructions). If the host configuration does not comply to the requirement
# you should call one of the functions "check_warning" or "check_failure"
# (depending on the severity of the problem).

# Check whether the filesystem Netkit is running from is capable of handling sparse
# files.

check_message "Checking filesystem type... "

FS_TYPE=`stat -f . | grep -w Type`
FS_TYPE=${FS_TYPE#*Type: }

if echo $FS_TYPE | grep -vqE "(ext[2,3,4])|(ntfs)|(ntfs-3g)|(fuse)|(reiser)|(jfs)|(xfs)"; then
   echo "failed!"
   echo
   echo "*** Warning: You appear to be running Netkit on a filesystem ($FS_TYPE)"
   echo "that does not support sparse files. This may result in dramatic performance"
   echo "loss and disk space consumption. It is strongly advised to run Netkit"
   echo "on a filesystem that supports sparse files (e.g., extX, NTFS, ReiserFS do;"
   echo "FAT32 does not)."
   echo
   check_warning
else
   echo "passed."
fi

