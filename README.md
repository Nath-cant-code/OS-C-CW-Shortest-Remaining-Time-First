# Operating Systems and Concurrency Coursework
# Shortest Remaining Time First Process Scheduler
This is the coursework group project for the Operating Systems and Concurrency module in Year 2 of Computer Science with Artificial Intelligence.

This group project was done by Group 41: Nathanael Ng, Muhammad Aatik Shaikh, and Shawn Chan

## Running the Progam on VS Code 

We built this project on VS Code, hence to run it on VS Code, you would only need to click the `Run Code` button on the top right corner.


## Running the Program in Terminal

### 1. Select Directory
Ensure that the current directory is changed to the directory of the program file. Copy the path of the file and enter the following command into the terminal:

`cd "path to file"`

### 2. Check for Compiler Installation
Ensure gcc is installed on your machine. To check, run 
```bash
gcc --version
``` 
in the terminal. If you do not have gcc installed, install it according to the requirements of your machine's operating system. 

### 3.  Compilation
This program uses POSIX threads, which might cause some issues on machines using Windows OS (the implementation of pthreads was done by Nathanael Ng on a macOS machine, hence there were no issues of any kind). To compile the program, run this in the terminal:

```bash 
gcc -std=c11 -Wall -Wextra -O2 Shortest_Remaining_Time_First.c -pthread -o srtf
```

If `-pthread` is not accepted on your Windows machine, run from WSL or MSYS2.

### 4. Run the program.

On macOS/Linux/WSL:
```bash
./srtf
```

On Windows:
```bash
.\srtf.exe
```
