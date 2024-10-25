#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <cstring>
#include <algorithm>
 
#define NUM_CHILDREN 4 // Number of child processes P1-P4
#define SHM_KEY 1234   // Key for shared memory
 
using namespace std;
 
// The user is asked wheter they want to manually input the values or use the random numbers
bool get_user_choice() {
    char choice;
    cout << "Would you like to manually enter the values for P1-P4? (y/n): ";
    cin >> choice;
    return (choice == 'y' || choice == 'Y');
}
 
// Function to get the values for P1-P4 from the user
void get_manual_input(int user_values[NUM_CHILDREN]) {
    for (int i = 0; i < NUM_CHILDREN; i++) {
        cout << "Enter value for P" << i + 1 << ": ";
        cin >> user_values[i]; // Read the user input value for each process
    }
}
 
// Function to fork the child processes and send numbers to parent via pipes
void fork_processes(int pipes[NUM_CHILDREN][2], bool manual_input, int user_values[NUM_CHILDREN]) {
    pid_t pid; // stores the process ID of the child process
 
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (pipe(pipes[i]) == -1) {
            cerr << "Pipe failed!" << endl;
            exit(1); // print error and terminate the program with error if it fails
        }
 
        pid = fork(); // Fork the child process
        if (pid < 0) {
            cerr << "Fork failed!" << endl;
            exit(1); // print error and terminate the program with error if fork fails
        }
 
        if (pid == 0) { // Child process P1-P4
            close(pipes[i][0]); // Close the read end of the pipe
 
            int number;
            if (manual_input) {
                // Use the user-provided values
                number = user_values[i];
            } else {
                // Generate a random number for each child process
                srand(time(0) + getpid()); // initialize random seed
                number = rand() % 20;      // generate random number between 0 and 19
            }
 
            cout << "Child P" << i + 1 << " (PID: " << getpid() << ") generated number: " << number << endl;
 
            // Send the random number to parent through pipe
            write(pipes[i][1], &number, sizeof(number));
            close(pipes[i][1]); // Close write end of pipe
            exit(0);            // Child process exits
        }
    }
}
 
// Function for parent (init) to read numbers from child processes
void read_from_pipes(int pipes[NUM_CHILDREN][2], int random_numbers[NUM_CHILDREN]) {
    for (int i = 0; i < NUM_CHILDREN; i++) {
        close(pipes[i][1]); // Close write end of pipe
 
        // Wait for the child to finish before reading from the pipe
        wait(nullptr); // Parent waits for the child process to complete
 
        // Read the random number from child process
        read(pipes[i][0], &random_numbers[i], sizeof(random_numbers[i]));
        close(pipes[i][0]); // Close read end of pipe
 
        cout << "Parent (init): Received number " << random_numbers[i] << " from child P" << i + 1 << endl;
    }
}
 
// Function to create a shared memory for communicating between init and scheduler
int create_shared_memory() {
    // Create shared memory segment
    int shmid = shmget(SHM_KEY, NUM_CHILDREN * sizeof(int), IPC_CREAT | 0666);
    if (shmid < 0) {
        cerr << "Shared memory creation failed!" << endl;
        exit(1);
    }
    return shmid;
}
 
// Function for init to write the numbers into shared memory
void write_to_shared_memory(int shmid, int random_numbers[NUM_CHILDREN]) {
    // Attach to shared memory
    int *shm_ptr = (int *)shmat(shmid, nullptr, 0);
    if (shm_ptr == (int *)-1) {
        cerr << "Shared memory attachment failed" << endl;
        exit(1);
    }
 
    // Write the random numbers to shared memory
    memcpy(shm_ptr, random_numbers, NUM_CHILDREN * sizeof(int));
    cout << "Parent (init): Wrote numbers to shared memory" << endl;
 
    // Detach init from shared memory
    shmdt(shm_ptr);
    cout <<  "Parent (init): Detached from shared memory" << endl;
}
 
// Scheduler process reads, sorts, and prints the numbers from shared memory
void scheduler_process(int shmid) {
    // Attach to shared memory
    int *shm_ptr = (int *)shmat(shmid, nullptr, 0);
    if (shm_ptr == (int *)-1) {
        cerr << "Scheduler: Shared memory attachment failed" << endl;
        exit(1);
    }
 
    // Read data from shared memory
    int numbers[NUM_CHILDREN];
    memcpy(numbers, shm_ptr, NUM_CHILDREN * sizeof(int)); // copy data from shared memory to local array
    cout << "Scheduler: Read numbers from shared memory\n";
 
    // Sort the numbers and print
    sort(numbers, numbers + NUM_CHILDREN);
    cout << "Scheduler: Sorted numbers: ";
    for (int i = 0; i < NUM_CHILDREN; i++) {
        cout << numbers[i] << " ";
    }
    cout << endl;
 
    // Detach and remove the shared memory
    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, nullptr); // Delete the shared memory
    cout << "Scheduler: Shared memory detached and deleted" << endl;
}
 
// Main function
int main() {
    int pipes[NUM_CHILDREN][2];    // Pipes for inter-process communication
    int random_numbers[NUM_CHILDREN];
    int user_values[NUM_CHILDREN]; // Array to store user input
 
    // Ask user if they want to input their own values for P1-P4
    bool manual_input = get_user_choice();
 
    // Get manual input if the user chooses to enter their own values
    if (manual_input) {
        get_manual_input(user_values);
    }
 
    // Fork child processes P1-P4
    fork_processes(pipes, manual_input, user_values);
 
    // Parent (init) reads random numbers from pipes
    read_from_pipes(pipes, random_numbers);
 
    // Create shared memory
    int shmid = create_shared_memory();
 
    // Parent (init) writes numbers to shared memory
    write_to_shared_memory(shmid, random_numbers);
 
    // Fork a separate scheduler process
    pid_t scheduler_pid = fork();
 
    if (scheduler_pid < 0) {
        cerr << "Fork failed for scheduler process!" << endl;
        exit(1);
    }
 
    if (scheduler_pid == 0) {
        // Scheduler process
        scheduler_process(shmid);
        exit(0);
    }
 
    // Wait for all children to finish
    for (int i = 0; i < NUM_CHILDREN; i++) {
        wait(nullptr); // Wait for each child to finish
    }
 
    // Wait for the scheduler process to finish
    waitpid(scheduler_pid, nullptr, 0);
 
    cout << "Parent (init): All child processes and scheduler completed!" << endl;
 
    return 0;
}