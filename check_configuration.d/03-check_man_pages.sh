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

# Check whether Netkit man pages are reachable

check_message "Checking for availability of man pages... "

if ! echo "${MANPATH}" | grep -qE "(:$NETKIT_HOME\/man\/?$)|(^$NETKIT_HOME\/man\/?:)|(:$NETKIT_HOME\/man\/?:)"; then
   echo "failed!"
   echo
   if [ -z "$MANPATH" ]; then
      NEW_MANPATH=":${NETKIT_HOME%/}/man"
   else
      NEW_MANPATH="\$MANPATH::${NETKIT_HOME%/}/man"
   fi
   echo "*** Warning: the environment variable MANPATH is not properly set. This"
   echo "will make man pages inaccessible. You should set it to the following"
   echo "value (all colons must be included):"
   echo
   echo $NEW_MANPATH
   echo
   echo "You can use one of the following commands, depending on the shell you"
   echo "are using:"
   echo
   echo "(for bash) export MANPATH=$NEW_MANPATH"
   echo "(for csh)  setenv MANPATH $NEW_MANPATH"
   echo
   check_warning
else
   echo "passed."
fi
