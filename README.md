# vxbootstrap
An architecture-specific bootstrap program that arbitrarily injects a list of libraries into a suspended process.
Usage:

vxbootstrap32/64.exe vxcmd=start vxexe=PATH_TO_EXE vxlibs=PATH_TO_LIBS [vxwd=PATH_TO_WD] [ARGS...]

vxbootstrap32/64.exe vxcmd=inject vxlibs=PATH_TO_LIBS vxpid=PID vxtid=TID vxls=0/1

Arguments:
- vxcmd=start: starts a new process.
- vxcmd=inject: injects a running process based on pid/tid.
- vxexe: Path to a target executable.
- vxlibs: A semicolon-delimited list of libraries to sideload.
- vxwd: An alternate working-directory (default is the path of the target exe)
- vxpid: A target Process ID
- vxtid: A target Thread ID
- vxls: Boolean to leave the process suspended after injection.
- ARGS: Any additional arguments will pass to the target process.