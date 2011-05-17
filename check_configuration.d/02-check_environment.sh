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

# Check whether NETKIT_HOME has been set

check_message "Checking environment... "

if [ -z "$NETKIT_HOME" ]; then
   echo "failed!"
   echo
   echo "*** Error: the environment variable NETKIT_HOME is not set. You should"
   echo "set it to the following value:"
   echo
   echo $PWD
   echo
   echo "You can use one of the following commands, depending on the shell you"
   echo "are using:"
   echo
   echo "(for bash) export NETKIT_HOME=$PWD"
   echo "(for csh)  setenv NETKIT_HOME $PWD"
   echo
   check_failure
else
   if [ "$NETKIT_HOME" != "$PWD" -a "$NETKIT_HOME" != "${PWD}/" ]; then
      echo "failed!"
      echo
      echo "*** Error: the environment variable NETKIT_HOME currently points at the"
      echo "wrong directory:"
      echo
      echo "(current value) $NETKIT_HOME"
      echo "(should be)     $PWD"
      echo
      echo "In order to fix this, you can use one of the following commands, depending"
      echo "on the shell you are using:"
      echo
      echo "(for bash) export NETKIT_HOME=$PWD"
      echo "(for csh)  setenv NETKIT_HOME $PWD"
      echo
      check_failure
   else
      echo "passed."
   fi
fi

NETKIT_BIN_DIR=$NETKIT_HOME/bin
