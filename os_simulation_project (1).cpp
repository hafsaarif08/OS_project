// Full OS Simulation with Hybrid Scheduling and Deadlock Management
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <map>
#include <set>
#include <iomanip>
#include <thread>
#include <chrono>
using namespace std;

struct Process {
    int pid, arrival, burst, remaining, priority;
    int waiting = 0, turnaround = 0, finishTime = 0;
    bool finished = false;
    vector<int> resourcesRequested;
};

struct Resource {
    int rid, total, available;
};

class OSSimulator {
private:
    vector<Process> processes;
    map<int, Resource> resources;
    map<int, vector<int>> allocation; // pid -> resources held
    map<int, vector<int>> request;    // pid -> resources requested
    int time = 0;
    int contextSwitches = 0;
    vector<pair<int, int>> ganttChart; // (pid, duration)
    vector<int> finishedPIDs;
    int deadlocksDetected = 0;

public:
    void inputProcesses() {
        int n;
        cout << "Enter number of processes: ";
        cin >> n;
        for (int i = 0; i < n; ++i) {
            Process p;
            p.pid = i;
            cout << "\nEnter details for Process " << i << ":\nArrival Time: ";
            cin >> p.arrival;
            cout << "Burst Time: ";
            cin >> p.burst;
            p.remaining = p.burst;
            cout << "Priority (lower = higher priority): ";
            cin >> p.priority;
            int k;
            cout << "Number of resources requested: ";
            cin >> k;
            cout << "Enter resource IDs: ";
            for (int j = 0; j < k; ++j) {
                int r;
                cin >> r;
                p.resourcesRequested.push_back(r);
                request[i].push_back(r);
            }
            processes.push_back(p);
        }
    }

    void inputResources() {
        int m;
        cout << "Enter number of resource types: ";
        cin >> m;
        for (int i = 0; i < m; ++i) {
            Resource r;
            r.rid = i;
            cout << "Total units of Resource " << i << ": ";
            cin >> r.total;
            r.available = r.total;
            resources[i] = r;
        }
    }

    void simulate(int quantum = 3) {
        queue<int> ready;
        int n = processes.size();
        set<int> inQueue;

        while (true) {
            // Add new arrivals
            for (int i = 0; i < n; ++i) {
                if (!processes[i].finished && processes[i].arrival <= time && inQueue.find(i) == inQueue.end()) {
                    ready.push(i);
                    inQueue.insert(i);
                }
            }

            if (ready.empty()) {
                if (all_of(processes.begin(), processes.end(), [](Process& p) { return p.finished; }))
                    break;
                time++;
                continue;
            }

            int pid = selectSchedulingPolicy(ready);
            inQueue.erase(pid);
            Process &p = processes[pid];

            int execTime = min(quantum, p.remaining);
            p.remaining -= execTime;
            time += execTime;
            contextSwitches++;
            ganttChart.push_back({pid, execTime});

            if (p.remaining <= 0) {
                p.finished = true;
                p.finishTime = time;
                p.turnaround = time - p.arrival;
                p.waiting = p.turnaround - p.burst;
                finishedPIDs.push_back(pid);
            } else {
                ready.push(pid);
                inQueue.insert(pid);
            }

            if (detectDeadlock()) {
                resolveDeadlock();
            }
        }
    }

    int selectSchedulingPolicy(queue<int>& ready) {
        vector<int> candidates;
        while (!ready.empty()) {
            candidates.push_back(ready.front());
            ready.pop();
        }

        // Dynamic selection: short queue -> SJN, medium -> Priority, long -> RR
        if (candidates.size() <= 2) {
            sort(candidates.begin(), candidates.end(), [&](int a, int b) {
                return processes[a].remaining < processes[b].remaining;
            });
        } else if (candidates.size() <= 5) {
            sort(candidates.begin(), candidates.end(), [&](int a, int b) {
                return processes[a].priority < processes[b].priority;
            });
        } // else keep as RR (FIFO)

        int selected = candidates.front();
        for (int pid : candidates) ready.push(pid);
        return selected;
    }

    bool detectDeadlock() {
        // Simple deadlock detection by checking circular waiting
        set<int> visited;
        for (auto& r : request) {
            for (int rid : r.second) {
                if (resources[rid].available == 0) {
                    visited.insert(r.first);
                }
            }
        }
        if (!visited.empty()) {
            cout << "\n[!] Deadlock detected among processes: ";
            for (int pid : visited) cout << pid << " ";
            cout << endl;
            deadlocksDetected++;
            return true;
        }
        return false;
    }

    void resolveDeadlock() {
        // Simply kill a process (first in list)
        for (Process& p : processes) {
            if (!p.finished) {
                cout << "[-] Terminating Process " << p.pid << " to resolve deadlock.\n";
                p.finished = true;
                return;
            }
        }
    }

    void drawGanttChart() {
        cout << "\nGantt Chart:\n";
        for (auto& g : ganttChart) {
            cout << "| P" << g.first << "(" << g.second << ") ";
        }
        cout << "|\n";
    }

    void printPerformance() {
        cout << "\nProcess Summary:\n";
        cout << "PID\tArrival\tBurst\tWaiting\tTurnaround\n";
        for (auto& p : processes) {
            cout << p.pid << "\t" << p.arrival << "\t" << p.burst << "\t"
                 << p.waiting << "\t" << p.turnaround << endl;
        }
        cout << "\nContext Switches: " << contextSwitches << endl;
        cout << "Deadlocks Detected and Resolved: " << deadlocksDetected << endl;
    }

    void drawRAG() {
        cout << "\nResource Allocation Graph (RAG):\n";
        for (auto& [pid, resList] : request) {
            for (int r : resList) {
                cout << "P" << pid << " --> R" << r << endl;
            }
        }
        for (auto& [pid, resHeld] : allocation) {
            for (int r : resHeld) {
                cout << "R" << r << " --> P" << pid << endl;
            }
        }
    }
};

int main() {
    OSSimulator sim;
    sim.inputResources();
    sim.inputProcesses();
    sim.simulate();
    sim.drawGanttChart();
    sim.printPerformance();
    sim.drawRAG();
    return 0;
}
