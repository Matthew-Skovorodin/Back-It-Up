# Back-It-Up
Creates file backups in a Linux environment using multi-threading and recursive subdirectory handling.

## Behavior
The program creates a directory .backup/ in the current working directory if one does not already exist. It creates a copy of all the regular files in the cwd into the .backup/ directory (all backup files have .bak appended to their name). 
If backup file exists, last modification times are compared to determine if backup is required. A new thread is allocated to copy each file. Each subdirectory is recusively handled. 
(If called with the optional -r (restore) argument, program restores all backup files in the .backup direcetory by copying them to the cwd.)

### Build:
To compile the program, use the following command:
```bash
make all
```

### Run:
Execute the compiled program with:
```bash
./BackItUp      (to backup)
./BackItUp -r   (to restore)
```

### Clean
To remove the compiled files, run:
```bash
make clean
```



