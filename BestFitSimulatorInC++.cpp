#include <iostream>
#include <vector>
#include <iomanip>
#include <string>
#include <climits>
#include <limits> // Added for INT_MAX to replace magic numbers
using namespace std;

// Struct to represent a memory partition (a block of memory)
struct Partition {
    int id;              // Unique identifier for the partition (e.g., 0, 1, 2...)
    int size;            // Total size of the partition (e.g., in KB or units)
    bool isFree;         // True if the partition is free (available), false if allocated
    int jobNumber;       // The job ID assigned to this partition (-1 if free)
    int jobSize;         // The size of the job allocated here (0 if free)
    int internalFragment; // Wasted space in this partition (size - jobSize; 0 if free)
};

// Struct to represent a job (a process requesting memory)
struct Job {
    int jobNumber; // Unique ID for the job (auto-incremented)
    int jobSize;   // Memory size required by the job
};

// Global vectors to store data:
// - memory: List of all partitions (the main memory pool)
// - waitingQueue: Jobs waiting for allocation if no suitable partition is free
// - deallocatedJobs: Jobs that have been deallocated (for historical tracking)
vector<Partition> memory;
vector<Job> waitingQueue;
vector<Job> deallocatedJobs;

// Function to display the current status of memory, including a table and metrics
void showStatus() {
    // Define column widths as constants for better readability and maintainability
    const int col1 = 12, col2 = 12, col3 = 12, col4 = 12, col5 = 12, col6 = 18;
    const int space = 2; // Space between columns

    // Calculate total table width for drawing borders
    int tableWidth = col1 + col2 + col3 + col4 + col5 + col6 + (5 * space);

    // Lambda function to print a horizontal line (used for table borders)
    auto line = [&](char ch) {
        for (int i = 0; i < tableWidth; i++) cout << ch;
        cout << "\n";
    };

    cout << "\n";
    line('='); // Top border

    // Print table header with column names
    cout << left
         << setw(col1) << "Part. ID" << setw(space) << ""
         << setw(col2) << "Size" << setw(space) << ""
         << setw(col3) << "Status" << setw(space) << ""
         << setw(col4) << "Job No." << setw(space) << ""
         << setw(col5) << "Job Size" << setw(space) << ""
         << setw(col6) << "Int.Fragment"
         << "\n";

    line('-'); // Separator line

    // Variables to calculate totals for metrics
    int totalIF = 0;     // Total internal fragmentation (sum of wasted space in used partitions)
    int usedCount = 0;   // Number of used partitions (for average calculation)

    // Loop through each partition and print its details in the table
    for (auto &p : memory) {
        // If the partition is used, add its fragmentation to the total and increment used count
        if (!p.isFree) {
            totalIF += p.internalFragment;
            usedCount++;
        }

        // Print the row: ID, Size, Status, Job Number (or -1 if free), Job Size (or -1), Fragmentation (or 0)
        cout << left
             << setw(col1) << p.id << setw(space) << ""
             << setw(col2) << p.size << setw(space) << ""
             << setw(col3) << (p.isFree ? "FREE" : "USED") << setw(space) << ""
             << setw(col4) << (p.isFree ? "FREE": to_string(p.jobNumber)) << setw(space) << ""
             << setw(col5) << (p.isFree ? "FREE" : to_string(p.jobSize)) << setw(space) << ""
             << setw(col6) << (p.isFree ? 0 : p.internalFragment)
             << "\n";
    }

    line('-'); // Separator line

    // Print total internal fragmentation, aligned under the last column
    cout << setw(col1 + col2 + col3 + col4 + col5 + (5 * space)) << ""
         << "Total: " << totalIF << "\n";

    line('='); // Bottom border

    // Display waiting queue: List jobs waiting for allocation
    cout << "\nWaiting Queue: ";
    if (waitingQueue.empty()) cout << "None";
    else {
        for (auto &j : waitingQueue)
            cout << "[Job " << j.jobNumber << " (" << j.jobSize << ")] ";
    }

    // Display deallocated jobs: List jobs that have been freed
    cout << "\nDeallocated Jobs: ";
    if (deallocatedJobs.empty()) cout << "None";
    else {
        for (auto &j : deallocatedJobs)
            cout << "[Job " << j.jobNumber << "] ";
    }

    // Calculate and display average internal fragmentation (as percentage)
    // Avoid division by zero if no partitions are used
    double avgInternal = (usedCount == 0 ? 0 : (double)totalIF / usedCount);

    cout << "\nAverage Internal Fragmentation: "
         << fixed << setprecision(2) << avgInternal;

    // Calculate and display memory utilization (average percentage of partitions used)
    // For each partition, if used, add (jobSize / size) * 100; then divide by total partitions
    double utilization = 0.0;
    for (auto &p : memory) {
        if (!p.isFree) {
            utilization += ((double)p.jobSize / p.size) * 100;
        }
    }
    utilization /= memory.size(); // Average across all partitions

    cout << "\nMemory Utilization: "
         << fixed << setprecision(2) << utilization << " %\n";

    line('='); // Final border
}

// Function to allocate a job using Best Fit algorithm
void allocateJob(Job job) {
    int bestIndex = -1;     // Index of the best-fitting partition (-1 if none found)
    int smallestFit = INT_MAX; // Smallest leftover space (using INT_MAX instead of magic number 999999)

    // Loop through partitions to find the best fit (smallest leftover space)
    for (int i = 0; i < memory.size(); i++) {
        if (memory[i].isFree && memory[i].size >= job.jobSize) { // Must be free and large enough
            int leftover = memory[i].size - job.jobSize; // Calculate leftover space
            if (leftover < smallestFit) { // Update if this is a better fit
                smallestFit = leftover;
                bestIndex = i;
            }
        }
    }

    // If no suitable partition found, add job to waiting queue
    if (bestIndex == -1) {
        cout << "\nNo available partition for Job " << job.jobNumber
             << " → Added to waiting queue.\n";
        waitingQueue.push_back(job);
        return;
    }

    // Allocate the job to the best partition
    memory[bestIndex].isFree = false; // Mark as used
    memory[bestIndex].jobNumber = job.jobNumber;
    memory[bestIndex].jobSize = job.jobSize;
    memory[bestIndex].internalFragment = memory[bestIndex].size - job.jobSize; // Calculate waste

    cout << "\nJob " << job.jobNumber << " allocated to Partition "
         << memory[bestIndex].id << " (Best Fit).\n";
}

// Function to try allocating jobs from the waiting queue (called after deallocation)
void tryAllocateWaiting() {
    if (waitingQueue.empty()) return; // Nothing to do if queue is empty

    vector<Job> remaining; // Temporary list for jobs that still can't be allocated

    // Process each waiting job
    for (auto &j : waitingQueue) {
        int bestIndex = -1, small = INT_MAX; // Using INT_MAX instead of magic number 999999

        for (int i = 0; i < memory.size(); i++) {
            if (memory[i].isFree && memory[i].size >= j.jobSize) {
                int leftover = memory[i].size - j.jobSize;
                if (leftover < small) {
                    small = leftover;
                    bestIndex = i;
                }
            }
        }

        // If allocation succeeds, update partition and print message
        if (bestIndex != -1) {
            memory[bestIndex].isFree = false;
            memory[bestIndex].jobNumber = j.jobNumber;
            memory[bestIndex].jobSize = j.jobSize;
            memory[bestIndex].internalFragment = memory[bestIndex].size - j.jobSize;

            cout << "\nWaiting Job " << j.jobNumber
                 << " allocated to Partition " << memory[bestIndex].id << ".\n";
        } else {
            // If still no fit, keep in remaining list
            remaining.push_back(j);
        }
    }

    // Update waiting queue with remaining jobs
    waitingQueue = remaining;
}

// Function to deallocate a job from its partition
void deallocateJob(int jobNumber) {
    // Search for the partition with the matching job
    for (auto &p : memory) {
        if (!p.isFree && p.jobNumber == jobNumber) { // Must be used and match job
            cout << "\nJob " << jobNumber << " deallocated from Partition "
                 << p.id << "\n";

            // Add to deallocated list for tracking
            deallocatedJobs.push_back({p.jobNumber, p.jobSize});

            // Reset partition to free state
            p.isFree = true;
            p.jobNumber = -1;
            p.jobSize = 0;
            p.internalFragment = 0;

            // Try to allocate waiting jobs now that space is free
            tryAllocateWaiting();
            return; // Exit after deallocating
        }
    }
    // If job not found, print error
    cout << "\nJob not found.\n";
}

// Main function: Sets up the simulation and runs the menu loop
int main() {
    int n; // Number of partitions
    cout << "Enter number of partitions: ";
    cin >> n;

    // Initialize partitions: Prompt for sizes with input validation (must be greater than zero)
    for (int i = 0; i < n; i++) {
        int s; // Size of partition
        do {
            cout << "Enter size of Partition " << i +1 << ": ";
            cin >> s;
            if (s <= 0) cout << "Invalid size. Try again.\n";
        } while (s <= 0); // Loop until valid positive size is entered
        memory.push_back({i + 1, s, true, -1, 0, 0});

    }

    int choice;       // User's menu choice
    int jobCounter = 1; // Counter for assigning unique job numbers

    // Menu loop: Continues until user chooses to exit
    do {
        cout << "\n========== BEST FIT MENU ==========\n";
        cout << "1. Add Job\n";
        cout << "2. Deallocate Job\n";
        cout << "3. Show Status\n";
        cout << "4. Exit\n";
        cout << "Choose: ";
        cin >> choice;

        if (choice == 1) { // Add a new job
            Job j;
            j.jobNumber = jobCounter++; // Assign and increment job number

            // Input validation for job size (must be > 0)
            do {
                cout << "Enter job size: ";
                cin >> j.jobSize;
                if (j.jobSize <= 0) cout << "Invalid size. Try again.\n";
            } while (j.jobSize <= 0); // Loop until valid positive size is entered

            allocateJob(j); // Attempt allocation
        }
        else if (choice == 2) { // Deallocate a job
            int jobNumber; // Renamed for consistency
            cout << "Enter job number to deallocate: ";
            cin >> jobNumber;
            deallocateJob(jobNumber); // Deallocate if found
        }
        else if (choice == 3) { // Show current status
            showStatus(); // Display table and metrics
        }
        // Choice 4 exits the loop
    } while (choice != 4);

    return 0; // End program
}
