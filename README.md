# 🧠 FCFS, RR, and MLFQ Scheduler Simulators (GTK+3 & C)

An interactive GUI-based simulator for OS scheduling algorithms built in C with GTK+3.  
Simulates **First-Come First-Serve**, **Round Robin**, and **Multilevel Feedback Queue (MLFQ)** with real-time memory and process management visuals.

---

## 🎯 Objective
To simulate a mini operating system’s CPU scheduler with memory and mutex control, using real scheduling algorithms and system calls — brought to life through an interactive GUI.

## 🎥 Demos

▶ [Watch FCFS Simulation](https://drive.google.com/file/d/1V_28KMTbfBvSoW5xL-juWEG2T-6fuOoD/view?usp=sharing)

▶ [Watch Round Robin Simulation](https://drive.google.com/file/d/1beHxHoqdIGuZJvuSPOluBnXJ0PTuK98z/view?usp=sharing)

▶ [Watch MLFQ Simulation](https://drive.google.com/file/d/18JnaF3otj3u_v0cKDZ4NiSgBadqma9Xz/view?usp=sharing)

---

## 🔍 Features

- **Dynamic Process Scheduling**: Simulates real-world CPU schedulers (FCFS, RR, MLFQ) from scratch
- **Interactive Execution**: Run step-by-step or in real-time with automatic cycle progression
- **Concurrent Process Management**: Visualizes Ready, Blocked, and Running states across all queues
- **Semaphore-Controlled Resources**: Implements mutexes for `userInput`, `userOutput`, and `file` access
- **Memory Visualization**: Real-time display of 60-word memory layout including PCBs, code, and variables
- **Priority-Based Blocking/Unblocking**: Unblocks processes based on priority level within the scheduler
- ⚙**Custom Quantum & Scheduler Selection**: Supports dynamic RR quantum & MLFQ feedback control
- **Live Execution Logs**: View executed instructions, resource status, and system events as they happen

---

## 📌 Instruction Syntax

```
assign x y         # Assign variable or input
print x            # Output variable
printFromTo x y    # Range printing
writeFile x y      # Save to file
readFile x         # Read from file
semWait resource   # Acquire mutex
semSignal resource # Release mutex
```

## ⚙️ How to Run

### 1. Install dependencies

```bash
sudo apt update
sudo apt install build-essential libgtk-3-dev pkg-config
```

### 2. Clone & run

```bash
git clone https://github.com/Masry05/OS-project.git
gcc controller.c backend.c gui.c utilities.c -o controller `pkg-config --cflags --libs gtk+-3.0`
./controller
```
---

## 📜 License
MIT License
