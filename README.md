# OS Jackfruit - Container Runtime

##  Team Members
- Akula Pragnya — PES1UG24AM028  
- Bhoomika Patil — PES1UG25AM801  

---

##  Overview
This project implements a lightweight container runtime using Linux kernel features such as namespaces, process management, IPC, memory monitoring, and scheduling.

---

##  1. Isolation Mechanisms

The runtime uses Linux namespaces to isolate system resources:

- **PID Namespace** → Separate process IDs  
- **UTS Namespace** → Separate hostname  
- **Mount Namespace** → Separate filesystem  

### Filesystem Isolation
- `chroot` → Restricts filesystem access  
- `pivot_root` → Replaces root filesystem  

> All containers share the host kernel, making them lightweight.

---

##  2. Supervisor & Lifecycle Management

The **engine (supervisor)** manages container execution.

### Responsibilities
- Create containers (`fork`, `exec`, `clone`)
- Track process metadata
- Handle signals (`SIGTERM`, `SIGKILL`)
- Reap zombie processes

---

##  3. IPC & Synchronization

### IPC Used
- Pipes
- Message queues
- Shared memory

### Synchronization
- Mutexes → Prevent race conditions  
- Semaphores / Condition Variables → Coordinate access  

---

##  4. Memory Management

### RSS (Resident Set Size)
Includes:
- Code, stack, heap in RAM  

Excludes:
- Swapped memory  

### Limits
- Soft Limit → Flexible  
- Hard Limit → Strict  

> Enforced at kernel level for reliability.

---

##  5. Scheduling Behavior

Uses Linux **Completely Fair Scheduler (CFS)**.

### Observations
- CPU-bound → High CPU usage  
- I/O-bound → More responsive  
- Mixed → Dynamic scheduling  

---

##  6. Setup & Execution

### Prerequisites
```bash
sudo apt update
sudo apt install -y build-essential make gcc linux-headers-$(uname -r) git
```

### Clone Repository
```bash
git clone https://github.com/pragnyaakula2/OS-Jackfruit.git
cd OS-Jackfruit/boilerplate
```

### Build
```bash
make clean
make
```

### Load Module
```bash
sudo insmod jackfruit.ko
lsmod | grep jackfruit
dmesg | tail
```

### Start Supervisor
```bash
sudo ./engine
```

### Run Containers
```bash
sudo ./cli run c1
sudo ./cli run c2
```

### Run Workloads
```bash
cd workloads
./memory_hog
./io_pulse
```

Or:
```bash
sudo ./cli exec c1 ./memory_hog
sudo ./cli exec c2 ./io_pulse
```

### CLI Commands
```bash
sudo ./cli list
sudo ./cli stop c1
sudo ./cli logs c1
```

### Cleanup
```bash
sudo pkill engine
sudo rmmod jackfruit
make clean
```

---

##  7. Design Decisions & Tradeoffs

### Namespace Isolation
- Lightweight but less secure than VMs

### Supervisor Model
- Simple but lacks advanced orchestration

### IPC via Pipes
- Easy but limited scalability

### Memory Monitor
- Timer-based → simple but slightly inefficient

### Scheduling
- Uses nice values → easy demonstration

---

## 📊 . Scheduler Experiment Results

| Process | Nice Value | CPU Usage |
|--------|-----------|----------|
| High Priority | -10 | ~65% |
| Low Priority  | 10  | ~35% |

### Insight
Higher priority processes get more CPU time, but lower ones are not starved.

---

##  Conclusion

This project demonstrates:
- Container isolation
- Process lifecycle management
- Safe concurrency
- Kernel-level resource control
- Real-world scheduling behavior

---

