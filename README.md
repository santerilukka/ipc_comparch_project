# Inter-Process Communication Project DTEK2085-3002

This project is part of the **Computer Architectures and Operating Systems** course. The task involves implementing inter-process communication between two processes: `init` and `Scheduler`. The communication is facilitated using **pipes** and **shared memory** to pass data between child processes and the parent process.

## Project Overview

The program consists of the following:

- **Parent Process (init)**: Forks 4 child processes (`P1`-`P4`), each generating a random or user-provided number.
- **Child Processes (P1-P4)**: These processes send their generated numbers to the parent (`init`) using pipes.
- **Scheduler**: Is an independent process that creates shared memory, reads numbers written by the parent (`init`), sorts them, and displays the sorted list.

## Running the Project

This project is implemented in **C++** on a **Linux-based VM**.

To run the project:

1. Clone the repository:

   ```bash
   git clone https://github.com/santerilukka/ipc_comparch_project
   cd ipc_comparch_project
    ```

2. Compile the program:

    ```bash
    g++ -o ipc_project ipc_project.cpp
    ```

3. Then you can run:

    ```bash
    ./ipc_project
    ```


You will be asked to either manually input values or let the program randomly generate values for the child processes (P1-P4).

The parent (init) forks child processes, collects the generated numbers, and writes them to shared memory. The Scheduler then sorts and prints the numbers.