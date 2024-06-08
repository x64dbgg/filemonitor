# File Monitor

FileOpMonitor utilises kernelmode that hooks into the file system to monitor and log file operations, including creation, deletion, and renaming. The driver uses file system minifilter techniques to intercept these operations and logs details to the kernel debugger output.
