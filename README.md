# Team Members

Akula Pragnya - PES1UG24AM028
Bhoomika Patil - PES1UG25AM801
---

## 1. Isolation Mechanisms

The runtime achieves isolation using Linux namespaces and filesystem isolation techniques. Namespaces allow processes to have different views of system resources, creating the illusion of separate environments.

- **PID Namespace**: Provides each container with its own process ID space. Processes inside the container cannot see or interact with processes outside it.
- **UTS Namespace**: Isolates hostname and domain name, allowing each container to define its own system identity.
- **Mount Namespace**: Gives each container an independent filesystem view. Changes to mounts inside a container do not affect the host or other containers.

For filesystem isolation:
- **chroot** restricts a process to a subtree of the filesystem.
- **pivot_root** replaces the root filesystem entirely, providing stronger isolation.

Despite these mechanisms, all containers share the **host kernel**, including:
- The scheduler
- Memory management
- Device drivers

This shared kernel model is what makes containers lightweight compared to virtual machines.

---

## 2. Supervisor and Process Lifecycle

A long-running supervisor process is essential for managing container lifecycles.

- Linux uses a **parent-child process model**.
- When a child process terminates, it becomes a **zombie** until the parent reaps it using `wait()`.

The supervisor is responsible for:
- Creating container processes (fork/exec)
- Tracking metadata (PID, state, resource usage)
- Handling signals (SIGTERM, SIGKILL)
- Reaping child processes to prevent zombies

It acts as a central control point for:
- Starting containers
- Monitoring execution
- Cleaning up resources after termination

Signal propagation is also managed through this hierarchy, ensuring proper termination and control of container processes.

---

## 3. IPC, Threads, and Synchronization

The project uses multiple inter-process communication (IPC) mechanisms and a bounded-buffer logging system.

### IPC Mechanisms
- Pipes or message queues for communication between processes
- Shared memory for efficient data exchange

### Concurrency Challenges
Shared data structures introduce race conditions:
- Multiple producers writing simultaneously
- Consumers reading from empty buffers
- Data overwrites when buffers are full

### Synchronization Strategy
- **Mutexes** ensure mutual exclusion for shared data access
- **Condition variables or semaphores** coordinate producer-consumer behavior:
  - Producers wait when the buffer is full
  - Consumers wait when the buffer is empty

These mechanisms ensure:
- Data consistency
- Safe concurrent access
- Reliable logging

---

## 4. Memory Management and Enforcement

**RSS (Resident Set Size)** measures the actual physical memory used by a process, including:
- Code
- Stack
- Heap currently in RAM

It does not include:
- Swapped-out pages
- Accurately attributed shared memory
- Kernel memory

### Memory Limits
- **Soft limit**: A flexible threshold that the system tries to respect
- **Hard limit**: A strict cap that cannot be exceeded

Soft limits allow better resource utilization, while hard limits enforce strict control.

### Kernel-Level Enforcement
Memory enforcement must occur in the kernel because:
- The kernel controls physical memory allocation
- User-space monitoring can be bypassed or delayed
- Only the kernel can enforce limits consistently across all processes

---

## 5. Scheduling Behavior

The runtime behavior reflects how the Linux scheduler manages workloads.

Linux uses the **Completely Fair Scheduler (CFS)**, which distributes CPU time based on fairness.

### Observations from Experiments
- **CPU-bound workloads** consume significant CPU time and compete with each other
- **I/O-bound workloads** yield CPU frequently and appear more responsive
- Mixed workloads show different scheduling patterns depending on resource usage

### Scheduling Goals
- **Fairness**: Equal CPU distribution over time
- **Responsiveness**: Faster handling of interactive or short tasks
- **Throughput**: Maximizing total work completed

These behaviors explain variations observed during experiments, as the scheduler dynamically balances competing demands.

---

---

## 6. Build, Load, and Run Instructions

These steps assume a fresh Ubuntu 22.04/24.04 VM with build tools installed.

### Prerequisites

```bash
sudo apt update
sudo apt install -y build-essential make gcc linux-headers-$(uname -r) git
```

Clone the repository:

```bash
git clone https://github.com/pragnyaakula2/OS-Jackfruit.git
cd OS-Jackfruit/boilerplate
```

---

### Build the Project

```bash
make clean
make
```

This compiles the kernel module and user-space components.

---

### Load the Kernel Module

```bash
sudo insmod jackfruit.ko
```

Verify it is loaded:

```bash
lsmod | grep jackfruit
```

Check kernel logs (useful if something breaks, which it inevitably will at least once):

```bash
dmesg | tail
```

---

### Start the Supervisor

```bash
sudo ./engine
```

The supervisor runs as a long-lived process and manages container lifecycle and scheduling.

---

### Launch Containers

Example command (adjust based on your CLI implementation):

```bash
sudo ./cli run c1
sudo ./cli run c2
```

Each container is assigned a name and PID namespace.

---

### Run Workloads

Navigate to workloads directory (if separate):

```bash
cd workloads
```

Run example workloads:

```bash
./memory_hog
./io_pulse
```

Or from CLI:

```bash
sudo ./cli exec c1 ./memory_hog
sudo ./cli exec c2 ./io_pulse
```

---

### Using the CLI

Typical operations include:

```bash
sudo ./cli list          # list running containers
sudo ./cli stop c1      # stop container
sudo ./cli logs c1      # view logs
```

(Commands may vary slightly depending on implementation.)

---

### Unload Module and Cleanup

Stop all containers first:

```bash
sudo pkill engine
```

Unload kernel module:

```bash
sudo rmmod jackfruit
```

Clean build artifacts:

```bash
make clean
```

---

## Conclusion

This project demonstrates how operating system mechanisms such as namespaces, process management, synchronization, memory control, and scheduling work together to support containerized execution. The runtime leverages these kernel features to provide isolation, control, and efficiency while sharing the underlying system resources.

