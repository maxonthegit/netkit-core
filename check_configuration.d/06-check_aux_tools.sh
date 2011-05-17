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

# Check for the availability of some system commands and utilities which are
# required for Netkit to work properly

check_message "Checking for availability of auxiliary tools:"
echo

OK=1
TEMP_PATH=":$PATH"
# Always initialize this variable with a space character
PATH_TO_BE_USED=":"

SYSTEM_COMMANDS="awk basename date dirname find getopt grep head id kill ls lsof ps readlink wc"
NETKIT_UTILITIES="port-helper tunctl uml_mconsole uml_switch"

for CURRENT_COMMAND in $SYSTEM_COMMANDS $NETKIT_UTILITIES; do
   printf "\t%-13s: " $CURRENT_COMMAND
   
   COMMAND_DIR=`which $CURRENT_COMMAND 2>/dev/null`
   ADD_TO_PATH=0
   
   # If the requested executable has not been found in the PATH,
   # try looking in standard directories
   if [ -z "$COMMAND_DIR" ]; then
      COMMAND_DIR=`whereis -b $CURRENT_COMMAND`
      # Clean up the output of whereis
      COMMAND_DIR=${COMMAND_DIR##*$CURRENT_COMMAND:}
      COMMAND_DIR=${COMMAND_DIR##* }
      if [ ! -z "$COMMAND_DIR" -a -f "$COMMAND_DIR" ]; then
         echo -n "ok, not in PATH"
         ADD_TO_PATH=1
      fi
   else
      echo -n "ok"
   fi

   # At this point the command has been looked for both in the PATH and in
   # standard system directories. Either it does not exist (else case) or it
   # exists and is probably not in the PATH (then case).
   if [ ! -z "$COMMAND_DIR" -a -f "$COMMAND_DIR" ]; then
      COMMAND_PATH=${COMMAND_DIR%/${CURRENT_COMMAND}}
      if echo "$TEMP_PATH" | grep -qvE "(:$COMMAND_PATH$)|(^$COMMAND_PATH:)|(:$COMMAND_PATH:)" && echo "${PATH_TO_BE_USED}" | grep -qv ":$COMMAND_PATH:"; then
         # PATH does not contain the directory where the current command is
         # located. Moreover, such directory has not been inserted into the
         # list of candidate user signalled paths (PATH_TO_BE_USED).
         PATH_TO_BE_USED="$PATH_TO_BE_USED$COMMAND_PATH:"
      fi
      if [ "$FIXMODE" = "1" -a $ADD_TO_PATH -eq 1 ]; then
         # Clean the PATH_TO_BE_USED variable, so that no warning is presented
         # to the user about the unavailability of the current tool.
         PATH_TO_BE_USED=":"
         ln -fs "$COMMAND_DIR" "${NETKIT_HOME%/}/bin"
         echo -n " (fixed)"
      fi
      echo
   else
      # If the command has not been found even after looking into standard
      # system directories, then it is likely that it has not been installed at
      # all.
      echo "error: cannot find any executable for this tool"
      OK=0
   fi
done

if [ $OK = 0 ]; then
   echo "failed!"
   echo
   echo "*** Error: some of the standard tools needed for running Netkit were"
   echo "not found in your Linux installation. Either the tools are not installed"
   echo "or your PATH variable is not properly set. Please, install the tools"
   echo "or set the PATH variable, then try again."
   echo
   REPORT_SUCCESS=0
   check_failure
fi

if [ "$PATH_TO_BE_USED" != ":" ]; then
   echo "failed!"
   echo
   echo "*** Error: all the standard tools required by Netkit have been found on"
   echo "your system, but you should update the PATH environment variable"
   echo "with the following entries:"
   echo
   echo $PATH_TO_BE_USED
   echo
   PATH_TO_BE_USED=${PATH_TO_BE_USED%:}
   echo "In order to do this, you can use one of the following commands, depending"
   echo "on the shell you are using:"
   echo
   echo "(for bash) export PATH=\$PATH$PATH_TO_BE_USED"
   echo "(for csh)  setenv PATH \$PATH$PATH_TO_BE_USED"
   echo
   echo "Alternatively, you can run this script with the --fix option, which"
   echo "automatically creates symbolic links for missing tools inside the"
   echo "$NETKIT_BIN_DIR directory."
   check_failure
fi

if [ $OK != 0 -a "$PATH_TO_BE_USED" = ":" ]; then
   echo "passed."
fi
