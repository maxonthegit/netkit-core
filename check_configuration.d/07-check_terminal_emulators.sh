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

# Check whether there are available terminal emulators

check_message "Checking for availability of terminal emulator applications:"
echo

OK=0
TEMP_PATH=":$PATH"
# Always initialize this variable with a space character
PATH_TO_BE_USED=":"

XTERM_WARNING=0

TERMINAL_EMULATORS="xterm konsole gnome-terminal"

for CURRENT_COMMAND in $TERMINAL_EMULATORS; do
   printf "\t%-15s: " $CURRENT_COMMAND
   
   COMMAND_DIR=`which $CURRENT_COMMAND 2>/dev/null`
   ADD_TO_PATH=0
   
   # If the requested executable has not been found in the PATH,
   # try looking in standard directories
   if [ -z "$COMMAND_DIR" ]; then
      if [ "$CURRENT_COMMAND" = "xterm" ]; then
         XTERM_WARNING=1
      fi
      COMMAND_DIR=`whereis -b $CURRENT_COMMAND`
      # Clean up the output of whereis
      COMMAND_DIR=${COMMAND_DIR##*$CURRENT_COMMAND:}
      COMMAND_DIR=${COMMAND_DIR##* }
      if [ ! -z "$COMMAND_DIR" -a -f "$COMMAND_DIR" ]; then
         echo -n "found, but not in PATH"
         ADD_TO_PATH=1
      fi
   else
      echo -n "found"
      OK=1
   fi

   if [ ! -z "$COMMAND_DIR" -a -f "$COMMAND_DIR" ]; then
      COMMAND_PATH=${COMMAND_DIR%/${CURRENT_COMMAND}}
      if echo "$TEMP_PATH" | grep -qvE "(:$COMMAND_PATH$)|(^$COMMAND_PATH:)|(:$COMMAND_PATH:)" && echo "${PATH_TO_BE_USED}" | grep -qv ":$COMMAND_PATH:"; then
         # PATH does not contain the directory where the current command is
         # located. Moreover, such directory has not been inserted into the
         # list of candidate user signalled paths (PATH_TO_BE_USED).
         PATH_TO_BE_USED="$PATH_TO_BE_USED$COMMAND_PATH:"
      fi
      if [ "$FIXMODE" = "1" -a $ADD_TO_PATH -eq 1 ]; then
         # Clean the PATH_TO_BE_USED variable, in order to suppress
         # warnings concerning unavailability of the current tool.
         PATH_TO_BE_USED=":"
         ln -fs "$COMMAND_DIR" "$NETKIT_HOME/bin"
         echo -n " (fixed)"
         OK=1
      fi
      echo
   else
      echo "not found"
   fi
done

WARNING=0
if [ $OK = 1 -a $XTERM_WARNING = 1 ]; then
   echo "failed!"
   echo
   echo "Warning: either xterm is not installed on your system or it is not"
   echo "currently available in the PATH. Since xterm is the default terminal"
   echo "emulator application used by Netkit virtual machines, this may prevent"
   echo "them from starting up properly. Please refer to the Netkit"
   echo "documentation for information about using terminal emulators that are"
   echo "different from xterm."
   echo
   WARNING=1
   check_warning
fi

if [ $OK = 0  -a "$PATH_TO_BE_USED" = ":" ]; then
   echo "failed!"
   echo
   echo "Warning: none of the supported terminal emulator applications appears"
   echo "to be properly installed on your system. This means that you will not"
   echo "be able to run virtual machines inside separate windows. Netkit can"
   echo "still be used by redirecting virtual machines I/O to a text-mode"
   echo "console (see the Netkit documentation for information about how to do"
   echo "this). Please refer to the documentation of your Linux installation"
   echo "for instructions about installing terminal emulators and try again."
   echo
   WARNING=1
   check_warning
fi


if [ $OK = 0 -a "$PATH_TO_BE_USED" != ":" ]; then
   echo "failed!"
   echo
   echo "Warning: some terminal emulator applications have been found, but if"
   echo "you want to use them you should update the PATH environment variable"
   echo "with the following entries:"
   echo
   echo $PATH_TO_BE_USED
   echo
   PATH_TO_BE_USED=${PATH_TO_BE_USED%:}
   echo "In order to do this, you can use one of the following commands, depending"
   echo "on the shell you are using:"
   echo
   echo "(for bash) export PATH=\$PATH$PATH_TO_BE_USED"
   echo "(for tcsh) setenv PATH \$PATH$PATH_TO_BE_USED"
   echo
   echo "Alternatively, you can run this script with the --fix option, which"
   echo "automatically creates symbolic links for missing tools inside the"
   echo "$NETKIT_BIN_DIR directory."
   echo
   WARNING=1
   check_warning
fi

if [ $WARNING = 0 ]; then
   echo "passed."
fi
