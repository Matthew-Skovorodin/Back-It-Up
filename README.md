# Back-It-Up
Creates file backups in a Linux environment, using multi-threading and recursive subdirectory handling.

## Command-line Operation
01 - Compile the code:
run

02 - Run the program:
./BackItUp
(If called with the optional -r (restore) argument, program restores all backup files in the .backup direcetory by copying them to the cwd.)

03 - To clean up and remove the compiled binary
make clean

## Behavior
The program creates a directory .backup/ in the current working directory if one does not already exist. It creates a copy of all the regular files in the cwd into the .backup/ directory (all backup files have .bak appended to their name). 
If backup file exists, last modification times are compared to determine if backup is required. A new thread is allocated to copy each file. Each subdirectory is recusively handled. 
