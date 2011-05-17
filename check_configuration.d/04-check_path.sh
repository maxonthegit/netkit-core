#!/bin/false

#     Copyright 2004-2007 Massimo Rimondini
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

# Check whether the PATH environment variable is properly set

check_message "Checking for proper directories in the PATH... "

if ! echo "${PATH}" | grep -qE "(:$NETKIT_HOME\/bin\/?$)|(^$NETKIT_HOME\/bin\/?:)|(:$NETKIT_HOME\/bin\/?:)"; then
   echo "failed!"
   echo
   NEW_PATH="${NETKIT_HOME%/}/bin:\$PATH"
   echo "*** Error: the environment variable PATH is not properly set. This"
   echo "will prevent fundamental Netkit tools from being functional. You should"
   echo "set the variable to the following value (all colons must be included):"
   echo
   echo $NEW_PATH
   echo
   echo "You can use one of the following commands, depending on the shell you"
   echo "are using:"
   echo
   echo "(for bash) export PATH=$NEW_PATH"
   echo "(for csh)  setenv PATH $NEW_PATH"
   echo
   check_failure
else
   echo "passed."
fi
