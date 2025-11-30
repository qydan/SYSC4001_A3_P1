import os
import subprocess
import re
import glob
import pandas as pd

# map the scheduler names to the exe files
SCHEDULERS = {
    "RR": "interrupts_RR.exe",
    "EP": "interrupts_EP.exe",
    "EP_RR": "interrupts_EP_RR.exe"
}

# where the test cases are stored
TEST_DIR = "testing"

def parse_input_file(filepath):
    # helper to read the input txt file and get the real arrival times for each PID
    # this is needed for accurate turnaround/response calc
    arrival_times = {}
    with open(filepath, 'r') as f:
        for line in f:
            if line.strip():
                parts = line.strip().split(',')
                # expected format: PID, Size, Arrival, Burst, IO_Freq, IO_Dur
                if len(parts) >= 3:
                    pid = int(parts[0].strip())
                    arrival = int(parts[2].strip())
                    arrival_times[pid] = arrival
    return arrival_times

def parse_execution_log(log_content, true_arrivals):
    # goes through the execution output to figure out the metrics
    lines = log_content.strip().split('\n')
    
    # dictionary to store stats for each process
    proc_stats = {}
    
    # regex to match the table rows in the output file ie: "| # | # | STATE | STATE |"
    row_pattern = re.compile(r"\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\w+)\s*\|\s*(\w+)\s*\|")
    
    current_time = 0
    
    for line in lines:
        match = row_pattern.search(line)
        if match:
            time = int(match.group(1))
            pid = int(match.group(2))
            old_state = match.group(3)
            new_state = match.group(4)
            current_time = time
            
            # init stats for this pid if we haven't seen it yet
            if pid not in proc_stats:
                proc_stats[pid] = {
                    'finish': None,
                    'first_run': None,
                    'total_wait': 0,
                    'last_ready_entry': None
                }

            # check if this is the first time the process is running (for response time)
            if new_state == "RUNNING" and proc_stats[pid]['first_run'] is None:
                proc_stats[pid]['first_run'] = time

            # if the process is done, save the finish time
            if new_state == "TERMINATED":
                proc_stats[pid]['finish'] = time

            # calculating wait time (time spent sitting in READY)
            # if it enters ready, start the timer
            if new_state == "READY":
                proc_stats[pid]['last_ready_entry'] = time
            
            # if it leaves ready to run, stop the timer and add to total wait
            if old_state == "READY" and new_state == "RUNNING":
                if proc_stats[pid]['last_ready_entry'] is not None:
                    wait_duration = time - proc_stats[pid]['last_ready_entry']
                    proc_stats[pid]['total_wait'] += wait_duration
                    proc_stats[pid]['last_ready_entry'] = None

    # calculate the final averages
    total_turnaround = 0
    total_wait = 0
    total_response = 0
    count = 0

    for pid, stats in proc_stats.items():
        # only count processes that actually finished and exist in our input file
        if pid in true_arrivals and stats['finish'] is not None:
            arrival = true_arrivals[pid]
            
            # turnaround = end time - arrival time
            turnaround = stats['finish'] - arrival
            
            # response = first cpu time - arrival time
            response = stats['first_run'] - arrival
            
            # wait = total time spent in ready queue
            wait = stats['total_wait']

            total_turnaround += turnaround
            total_response += response
            total_wait += wait
            count += 1

    if count == 0:
        return 0, 0, 0, 0

    avg_turnaround = total_turnaround / count
    avg_wait = total_wait / count
    avg_response = total_response / count
    throughput = count / current_time if current_time > 0 else 0

    return throughput, avg_wait, avg_turnaround, avg_response

def run_analysis():
    # find all the .txt files in the testing directory recursively
    test_files = glob.glob(os.path.join(TEST_DIR, "**/*.txt"), recursive=True)
    
    # ignore the output files (execution*.txt) and cmake stuff, we just want inputs
    input_files = [f for f in test_files if "execution" not in f and "CMake" not in f]
    
    results_data = []

    # print the header for the console output
    print(f"{'Scheduler':<10} | {'Test Case':<10} | {'Thrpt':<8} | {'Avg Wait':<10} | {'Avg TAT':<10} | {'Avg Resp':<10}")
    print("-" * 75)

    for test_path in sorted(input_files):
        test_name = os.path.basename(os.path.dirname(test_path)) # grabs the folder name e.g. "test1"
        if not test_name.startswith("test"): continue 

        # get the real arrival times from the input file
        true_arrivals = parse_input_file(test_path)

        for sched_name, binary in SCHEDULERS.items():
            # make sure the exe actually exists before trying to run it
            if not os.path.exists(binary):
                # check if it needs ./ prefix (linux/mac stuff)
                if os.path.exists("./" + binary):
                    binary = "./" + binary
                else:
                    continue

            # run the simulation
            try:
                # suppress stdout so the console doesn't get flooded
                subprocess.run([binary, test_path], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                
                # figure out which output file to read based on the scheduler
                output_filename = f"execution{sched_name}.txt"
                
                if os.path.exists(output_filename):
                    with open(output_filename, 'r') as f:
                        log_content = f.read()
                    
                    # do the math
                    th, wait, tat, resp = parse_execution_log(log_content, true_arrivals)
                    
                    # print to console
                    print(f"{sched_name:<10} | {test_name:<10} | {th:.5f}  | {wait:8.2f}   | {tat:8.2f}   | {resp:8.2f}")
                    
                    # save data for the csv
                    results_data.append({
                        "Scheduler": sched_name,
                        "Test": test_name,
                        "Throughput": th,
                        "Avg Wait": wait,
                        "Avg TAT": tat,
                        "Avg Response": resp
                    })
                else:
                    print(f"Error: Couldn't find output file {output_filename}")

            except Exception as e:
                print(f"Error running {sched_name} on {test_name}: {e}")

    # dump everything to a csv so i can make charts later
    df = pd.DataFrame(results_data)
    df.to_csv("final_metrics.csv", index=False)
    print("\nResults saved to 'final_metrics.csv'")

if __name__ == "__main__":
    run_analysis()