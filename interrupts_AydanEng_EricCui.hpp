/**
 * @file interrupts.hpp
 * @author Aydan Eng, Eric Cui
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001 amd includes Assignment 1 and 2 code
 *
 */

#ifndef INTERRUPTS_HPP_
#define INTERRUPTS_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <tuple>
#include <random>
#include <utility>
#include <sstream>
#include <iomanip>
#include <algorithm>

// An enumeration of states to make assignment easier
enum states
{
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED,
    NOT_ASSIGNED
};
std::ostream &operator<<(std::ostream &os, const enum states &s)
{ // Overloading the << operator to make printing of the enum easier

    std::string state_names[] = {
        "NEW",
        "READY",
        "RUNNING",
        "WAITING",
        "TERMINATED",
        "NOT_ASSIGNED"};
    return (os << state_names[s]);
}

struct memory_partition
{
    unsigned int partition_number;
    unsigned int size;
    int occupied;
} memory_partitions[] = {
    {1, 40, -1},
    {2, 25, -1},
    {3, 15, -1},
    {4, 10, -1},
    {5, 8, -1},
    {6, 2, -1}};

struct PCB
{
    int PID;
    unsigned int size;
    unsigned int arrival_time;
    int start_time;
    unsigned int processing_time;
    unsigned int remaining_time;
    int partition_number;
    enum states state;
    unsigned int io_freq;
    unsigned int io_duration;

    unsigned int time_slice_time;
    unsigned int io_return_time;
    unsigned int time_since_io;
};

//-------------------------------CODE FROM ASSINGMENT 1 AND ASSIGNMENT 2------------------------------

const std::vector<std::string> VECTOR_TABLE = {
    "0X01E3",
    "0X029C",
    "0X0695",
    "0X042B",
    "0X0292",
    "0X048B",
    //don't need rest of the vectors for this assignment
};

// Boilerplate function for interrupts from assignment 1
std::pair<std::string, int> intr_boilerplate(int current_time, int intr_num, int context_save_time)
{
    std::string execution = "";

    execution += std::to_string(current_time) + ", " + std::to_string(1) + ", switch to kernel mode\n";
    current_time++;

    execution += std::to_string(current_time) + ", " + std::to_string(context_save_time) + ", context saved\n";
    current_time += context_save_time;

    char vector_address_c[10];
    sprintf(vector_address_c, "0x%04X", (intr_num * 2));
    std::string vector_address(vector_address_c);
    std::string isr_address = (intr_num < (int)VECTOR_TABLE.size()) ? VECTOR_TABLE[intr_num] : "0X0000";

    execution += std::to_string(current_time) + ", " + std::to_string(1) + ", find vector " + std::to_string(intr_num) + " in memory position " + vector_address + "\n";
    current_time++;

    execution += std::to_string(current_time) + ", " + std::to_string(1) + ", load address " + isr_address + " into the PC\n";
    current_time++;

    return std::make_pair(execution, current_time);
}

// CONTEXT SWIITCH
std::pair<std::string, int> context_switch(int current_time)
{
    auto [execution, time] = intr_boilerplate(current_time, 2, 10);
    execution += std::to_string(time) + ", 1, IRET\n";
    time += 1;
    return std::make_pair(execution, time);
}

// Note: No longer random IO delays (was random in A2) to properly compare RR, EP and EP_RR

// SYSCCALL
std::pair<std::string, int> system_call(int current_time)
{
    auto [execution, time] = intr_boilerplate(current_time, 3, 4);
    execution += std::to_string(time) + ", 1, obtain ISR address\n";
    time += 1;
    execution += std::to_string(time) + ", 2, Call device driver\n";
    time += 2;
    execution += std::to_string(time) + ", 1, Perform device check\n";
    time += 1;
    execution += std::to_string(time) + ", 1, Send device instruction\n";
    time += 1;
    execution += std::to_string(time) + ", 1, IRET\n";
    time += 1;
    return std::make_pair(execution, time);
}

// END_IO
std::pair<std::string, int> end_io(int current_time)
{
    auto [execution, time] = intr_boilerplate(current_time, 4, 4);
    execution += std::to_string(time) + ", 2, store information in memory\n";
    time += 2;
    execution += std::to_string(time) + ", 1, reset the io operation\n";
    time += 1;
    execution += std::to_string(time) + ", 1, Send standby instruction\n";
    time += 1;
    execution += std::to_string(time) + ", 1, IRET\n";
    time += 1;
    return std::make_pair(execution, time);
}

//------------------------------------HELPER FUNCTIONS FOR THE SIMULATOR------------------------------
// Following function was taken from stackoverflow; helper function for splitting strings
std::vector<std::string> split_delim(std::string input, std::string delim)
{
    std::vector<std::string> tokens;
    std::size_t pos = 0;
    std::string token;
    while ((pos = input.find(delim)) != std::string::npos)
    {
        token = input.substr(0, pos);
        tokens.push_back(token);
        input.erase(0, pos + delim.length());
    }
    tokens.push_back(input);

    return tokens;
}

// Function that takes a queue as an input and outputs a string table of PCBs
std::string print_PCB(std::vector<PCB> _PCB)
{
    const int tableWidth = 83;

    std::stringstream buffer;

    // Print top border
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+" << std::endl;

    // Print headers
    buffer << "|"
           << std::setfill(' ') << std::setw(4) << "PID"
           << std::setw(2) << "|"
           << std::setfill(' ') << std::setw(11) << "Partition"
           << std::setw(2) << "|"
           << std::setfill(' ') << std::setw(5) << "Size"
           << std::setw(2) << "|"
           << std::setfill(' ') << std::setw(13) << "Arrival Time"
           << std::setw(2) << "|"
           << std::setfill(' ') << std::setw(11) << "Start Time"
           << std::setw(2) << "|"
           << std::setfill(' ') << std::setw(14) << "Remaining Time"
           << std::setw(2) << "|"
           << std::setfill(' ') << std::setw(11) << "State"
           << std::setw(2) << "|" << std::endl;

    // Print separator
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+" << std::endl;

    // Print each PCB entry
    for (const auto &program : _PCB)
    {
        buffer << "|"
               << std::setfill(' ') << std::setw(4) << program.PID
               << std::setw(2) << "|"
               << std::setw(11) << program.partition_number
               << std::setw(2) << "|"
               << std::setw(5) << program.size
               << std::setw(2) << "|"
               << std::setw(13) << program.arrival_time
               << std::setw(2) << "|"
               << std::setw(11) << program.start_time
               << std::setw(2) << "|"
               << std::setw(14) << program.remaining_time
               << std::setw(2) << "|"
               << std::setw(11) << program.state
               << std::setw(2) << "|" << std::endl;
    }

    // Print bottom border
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+" << std::endl;

    return buffer.str();
}

// Overloaded function that takes a single PCB as input
std::string print_PCB(PCB _PCB)
{
    std::vector<PCB> temp;
    temp.push_back(_PCB);
    return print_PCB(temp);
}

std::string print_exec_header()
{

    const int tableWidth = 49;

    std::stringstream buffer;

    // Print top border
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+" << std::endl;

    // Print headers
    buffer << "|"
           << std::setfill(' ') << std::setw(18) << "Time of Transition"
           << std::setw(2) << "|"
           << std::setfill(' ') << std::setw(3) << "PID"
           << std::setw(2) << "|"
           << std::setfill(' ') << std::setw(10) << "Old State"
           << std::setw(2) << "|"
           << std::setfill(' ') << std::setw(10) << "New State"
           << std::setw(2) << "|" << std::endl;

    // Print separator
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+" << std::endl;

    return buffer.str();
}

std::string print_exec_status(unsigned int current_time, int PID, states old_state, states new_state)
{

    const int tableWidth = 49;

    std::stringstream buffer;

    buffer << "|"
           << std::setfill(' ') << std::setw(18) << current_time
           << std::setw(2) << "|"
           << std::setw(3) << PID
           << std::setw(2) << "|"
           << std::setw(10) << old_state
           << std::setw(2) << "|"
           << std::setw(10) << new_state
           << std::setw(2) << "|" << std::endl;

    return buffer.str();
}

std::string print_exec_footer()
{
    const int tableWidth = 49;
    std::stringstream buffer;

    // Print bottom border
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+" << std::endl;

    return buffer.str();
}

// Synchronize the process in the process queue
void sync_queue(std::vector<PCB> &process_queue, PCB _process)
{
    for (auto &process : process_queue)
    {
        if (process.PID == _process.PID)
        {
            process = _process;
        }
    }
}

// Writes a string to a file
void write_output(std::string execution, const char *filename)
{
    std::ofstream output_file(filename);

    if (output_file.is_open())
    {
        output_file << execution;
        output_file.close(); // Close the file when done
        std::cout << "File content overwritten successfully." << std::endl;
    }
    else
    {
        std::cerr << "Error opening file!" << std::endl;
    }

    std::cout << "Output generated in " << filename << ".txt" << std::endl;
}

//--------------------------------------------FUNCTIONS FOR THE "OS"-------------------------------------

// Assign memory partition to program
bool assign_memory(PCB &program)
{
    int size_to_fit = program.size;
    int available_size = 0;

    for (int i = 5; i >= 0; i--)
    {
        available_size = memory_partitions[i].size;

        if (size_to_fit <= available_size && memory_partitions[i].occupied == -1)
        {
            memory_partitions[i].occupied = program.PID;
            program.partition_number = memory_partitions[i].partition_number;
            return true;
        }
    }

    return false;
}

// Free a memory partition
bool free_memory(PCB &program)
{
    for (int i = 5; i >= 0; i--)
    {
        if (program.PID == memory_partitions[i].occupied)
        {
            memory_partitions[i].occupied = -1;
            program.partition_number = -1;
            return true;
        }
    }
    return false;
}

// Print the memory usage for the bonus task
void print_memory_usage(unsigned int current_time)
{
    int total_used = 0;
    int total_free = 0;
    std::cout << "[Memory at " << current_time << "ms] ";
    for (int i = 0; i < 6; i++)
    {
        if (memory_partitions[i].occupied != -1)
        {
            total_used += memory_partitions[i].size;
            std::cout << "[P" << memory_partitions[i].partition_number << ":" << memory_partitions[i].occupied << "] ";
        }
        else
        {
            total_free += memory_partitions[i].size; // Count free/usable memory
            std::cout << "[P" << memory_partitions[i].partition_number << ":FREE] ";
        }
    }
    std::cout << "\nTotal Used: " << total_used << "Mb";
    std::cout << "\nTotal Free (Usable): " << total_free << "Mb" << std::endl;
}

// Convert a list of strings into a PCB
PCB add_process(std::vector<std::string> tokens)
{
    PCB process;
    process.PID = std::stoi(tokens[0]);
    process.size = std::stoi(tokens[1]);
    process.arrival_time = std::stoi(tokens[2]);
    process.processing_time = std::stoi(tokens[3]);
    process.remaining_time = std::stoi(tokens[3]);
    process.io_freq = std::stoi(tokens[4]);
    process.io_duration = std::stoi(tokens[5]);
    process.start_time = -1;
    process.partition_number = -1;
    process.state = NOT_ASSIGNED;

    process.time_slice_time = 0;
    process.time_since_io = 0;
    process.io_return_time = 0;

    return process;
}

// Returns true if all processes in the queue have terminated
bool all_process_terminated(std::vector<PCB> processes)
{

    for (auto process : processes)
    {
        if (process.state != TERMINATED)
        {
            return false;
        }
    }

    return true;
}

// Terminates a given process
void terminate_process(PCB &running, std::vector<PCB> &job_queue)
{
    running.remaining_time = 0;
    running.state = TERMINATED;
    free_memory(running);
    sync_queue(job_queue, running);
}

// set the process in the ready queue to runnning
void run_process(PCB &running, std::vector<PCB> &job_queue, std::vector<PCB> &ready_queue, unsigned int current_time)
{
    running = ready_queue.back();
    ready_queue.pop_back();
    running.start_time = current_time;
    running.state = RUNNING;
    sync_queue(job_queue, running);
}

void idle_CPU(PCB &running)
{
    running.start_time = 0;
    running.processing_time = 0;
    running.remaining_time = 0;
    running.arrival_time = 0;
    running.io_duration = 0;
    running.io_freq = 0;
    running.partition_number = 0;
    running.size = 0;
    running.state = NOT_ASSIGNED;
    running.PID = -1;
}

#endif