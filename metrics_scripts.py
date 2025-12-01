import os
import subprocess
import re
import glob
import pandas as pd

TEST_DIR = "testing"

# helper to get arrival times from input file for accurate TAT calc
def get_arrivals(filepath):
    arrivals = {}
    with open(filepath, 'r') as f:
        for line in f:
            if not line.strip(): continue
            parts = line.split(',')
            if len(parts) >= 3:
                # PID is index 0, Arrival is index 2
                arrivals[int(parts[0])] = int(parts[2])
    return arrivals

def calc_stats(log_text, real_arrivals):
    lines = log_text.strip().split('\n')
    proc_data = {}
    
    # regex for the table row: | time | pid | old | new |
    row_regex = re.compile(r"\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\w+)\s*\|\s*(\w+)\s*\|")
    
    curr_t = 0
    
    for line in lines:
        m = row_regex.search(line)
        if m:
            t = int(m.group(1))
            pid = int(m.group(2))
            state_old = m.group(3)
            state_new = m.group(4)
            curr_t = t
            
            if pid not in proc_data:
                proc_data[pid] = {'finish': None, 'start': None, 'wait': 0, 'ready_entry': None}

            # capture first run time for response time
            if state_new == "RUNNING" and proc_data[pid]['start'] is None:
                proc_data[pid]['start'] = t

            if state_new == "TERMINATED":
                proc_data[pid]['finish'] = t

            # track wait time (time spent in READY)
            if state_new == "READY":
                proc_data[pid]['ready_entry'] = t
            
            if state_old == "READY" and state_new == "RUNNING":
                if proc_data[pid]['ready_entry'] is not None:
                    proc_data[pid]['wait'] += (t - proc_data[pid]['ready_entry'])
                    proc_data[pid]['ready_entry'] = None

    # calculate averages
    total_tat = 0
    total_wait = 0
    total_resp = 0
    n = 0

    for pid, d in proc_data.items():
        if pid in real_arrivals and d['finish'] is not None:
            arr = real_arrivals[pid]
            total_tat += (d['finish'] - arr)
            total_resp += (d['start'] - arr)
            total_wait += d['wait']
            n += 1

    if n == 0: return 0, 0, 0, 0

    return (n / curr_t if curr_t > 0 else 0), (total_wait / n), (total_tat / n), (total_resp / n)

def main():
    # recursively find all test txt files
    input_files = [f for f in glob.glob(f"{TEST_DIR}/**/*.txt", recursive=True) 
                   if "execution" not in f and "CMake" not in f]
    
    results = []
    print(f"{'Sched':<10} | {'Test':<10} | {'Thrpt':<8} | {'Avg Wait':<10} | {'Avg TAT':<10} | {'Avg Resp':<10}")
    print("-" * 75)

    schedulers = ["RR", "EP", "EP_RR"]

    for test_file in sorted(input_files):
        test_case = os.path.basename(os.path.dirname(test_file))
        if not test_case.startswith("test"): continue 

        arrivals = get_arrivals(test_file)

        for sched in schedulers:
            exe = f"interrupts_{sched}.exe"
            # handle ./ for linux/mac execution if needed
            cmd = f"./{exe}" if os.path.exists(f"./{exe}") else exe
            
            if not os.path.exists(cmd) and not os.path.exists(exe):
                continue

            try:
                subprocess.run([cmd, test_file], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                
                outfile = f"execution{sched}.txt"
                if os.path.exists(outfile):
                    with open(outfile, 'r') as f:
                        content = f.read()
                    
                    th, wait, tat, resp = calc_stats(content, arrivals)
                    print(f"{sched:<10} | {test_case:<10} | {th:.5f}  | {wait:8.2f}   | {tat:8.2f}   | {resp:8.2f}")
                    
                    results.append({
                        "Scheduler": sched,
                        "Test": test_case,
                        "Throughput": th,
                        "Avg Wait": wait,
                        "Avg TAT": tat,
                        "Avg Response": resp
                    })
            except Exception as e:
                print(f"Failed on {sched} / {test_case}: {e}")

    # export
    pd.DataFrame(results).to_csv("final_metrics.csv", index=False)
    print("\nDone. Saved to final_metrics.csv")

if __name__ == "__main__":
    main()
