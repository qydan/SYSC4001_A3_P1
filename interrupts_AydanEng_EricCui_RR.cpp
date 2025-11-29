/**
 * @file interrupts.cpp
 * @author Aydan Eng, Eric Cui
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 *
 */

#include <interrupts_AydanEng_EricCui.hpp>

// void FCFS(std::vector<PCB> &ready_queue)
// {
//     std::sort(
//         ready_queue.begin(),
//         ready_queue.end(),
//         [](const PCB &first, const PCB &second)
//         {
//             return (first.arrival_time > second.arrival_time);
//         });
// }

std::tuple<std::string /* add std::string for bonus mark */> run_simulation(std::vector<PCB> list_processes)
{

    std::vector<PCB> ready_queue; // The ready queue of processes
    std::vector<PCB> wait_queue;  // The wait queue of processes
    std::vector<PCB> job_list;    // A list to keep track of all the processes. This is similar
                                  // to the "Process, Arrival time, Burst time" table that you
                                  // see in questions. You don't need to use it, I put it here
                                  // to make the code easier :).
    std::vector<PCB> memory_wait_queue;
    unsigned int current_time = 0;
    PCB running;

    // Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    // make the output table (the header row)
    execution_status = print_exec_header();

    // Loop while till there are no ready or waiting processes.
    // This is the main reason I have job_list, you don't have to use it.
    while (!all_process_terminated(job_list) || !list_processes.empty())
    {

        // Inside this loop, there are three things you must do:
        //  1) Populate the ready queue with processes as they arrive
        //  2) Manage the wait queue
        //  3) Schedule processes from the ready queue

        // Population of ready queue is given to you as an example.
        // Go through the list of proceeses
        // for (auto &process : list_processes)
        // {
        //     if (process.arrival_time == current_time)
        //     { // check if the AT = current time
        //         // if so, assign memory and put the process into the ready queue
        //         assign_memory(process);

        //         process.state = READY;          // Set the process state to READY
        //         ready_queue.push_back(process); // Add the process to the ready queue
        //         job_list.push_back(process);    // Add it to the list of processes

        //         execution_status += print_exec_status(current_time, process.PID, NEW, READY);
        //     }
        // }

        auto iterator = list_processes.begin();
        while (iterator != list_processes.end())
        {
            if (iterator->arrival_time <= current_time)
            {
                PCB temporary = *iterator;
                if (assign_memory(temporary))
                {
                    temporary.state = READY;
                    ready_queue.push_back(temporary);
                    job_list.push_back(temporary);
                    execution_status += print_exec_status(current_time, temporary.PID, NEW, READY);
                    print_memory_usage(current_time);
                }
                else
                {
                    memory_wait_queue.push_back(temporary);
                    job_list.push_back(temporary);
                }
                iterator = list_processes.erase(iterator);
            }
            else
            {
                iterator += 1;
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        // This mainly involves keeping track of how long a process must remain in the ready queue
        auto wait_iterator = wait_queue.begin();
        while (wait_iterator != wait_queue.end())
        {
            if (wait_iterator->io_return_time <= current_time)
            {
                auto [log, new_time] = end_io(current_time);
                // execution_status += log;
                current_time = new_time;

                wait_iterator->state = READY;
                wait_iterator->time_slice_time = 0;
                ready_queue.push_back(*wait_iterator);
                sync_queue(job_list, *wait_iterator);
                execution_status += print_exec_status(current_time, wait_iterator->PID, WAITING, READY);
                wait_iterator = wait_queue.erase(wait_iterator);
            }
            else
            {
                wait_iterator += 1;
            }
        }
        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        // example of FCFS is shown here

        // RR
        if (running.PID != -1)
        {
            // check for expiration fo time slice (quantum = 100)
            if (running.time_slice_time >= 100)
            {
                if (!ready_queue.empty())
                {
                    auto [log, new_time] = context_switch(current_time);
                    // execution_status += log;
                    current_time = new_time;

                    running.state = READY;
                    running.time_slice_time = 0; // reset quantum
                    sync_queue(job_list, running);
                    ready_queue.push_back(running); // move to back of queue
                    execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                    idle_CPU(running);
                }
                else
                {
                    // no other process else is ready, reset quantum and continue
                    running.time_slice_time = 0;
                }
            }
        }

        if (running.PID == -1 && !ready_queue.empty())
        {
            auto [log, new_time] = context_switch(current_time);
            // execution_status += log;
            current_time = new_time;

            running = ready_queue.front();
            ready_queue.erase(ready_queue.begin());
            running.state = RUNNING;
            running.time_slice_time = 0; // Ensure quantum starts at 0
            sync_queue(job_list, running);
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
        }

        /////////////////////////////////////////////////////////////////

        // execution
        if (running.PID != -1)
        {
            current_time += 1;
            running.remaining_time -= 1;
            running.time_since_io += 1;
            running.time_slice_time += 1;

            // termination
            if (running.remaining_time == 0)
            {
                running.state = TERMINATED;
                sync_queue(job_list, running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
                free_memory(running);
                idle_CPU(running);

                // mem wait queue
                auto mem_it = memory_wait_queue.begin();
                while (mem_it != memory_wait_queue.end())
                {
                    if (assign_memory(*mem_it))
                    {
                        mem_it->state = READY;
                        ready_queue.push_back(*mem_it);
                        sync_queue(job_list, *mem_it);
                        execution_status += print_exec_status(current_time, mem_it->PID, NEW, READY);
                        print_memory_usage(current_time);
                        mem_it = memory_wait_queue.erase(mem_it);
                    }
                    else
                    {
                        mem_it += 1;
                    }
                }
            }

            // IO Request
            else if (running.io_freq > 0 && running.time_since_io >= running.io_freq)
            {
                auto [log, new_time] = system_call(current_time);
                // execution_status += log;
                current_time = new_time;

                running.state = WAITING;
                running.io_return_time = current_time + running.io_duration;
                running.time_since_io = 0;
                sync_queue(job_list, running);
                wait_queue.push_back(running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);
                idle_CPU(running);
            }
        }
        else
        {
            current_time += 1;
        }
    }

    // Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}

int main(int argc, char **argv)
{

    // Get the input file from the user
    if (argc != 2)
    {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    // Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    // Ensure that the file actually opens
    if (!input_file.is_open())
    {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    // Parse the entire input file and populate a vector of PCBs.
    // To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while (std::getline(input_file, line))
    {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    // With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "executionRR.txt");

    return 0;
}