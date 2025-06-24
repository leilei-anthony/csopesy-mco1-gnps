# csopesy-mco1-gnps

## How to Run

```bash
# 1. Edit the configuration file
edit config.txt

# 2. Navigate to the project directory
cd directory

# 3. Build the project using MinGW Make
mingw32-make

# 4. Run the executable
./csopesy.exe
```

# Config Settings

For Round-Robin scheduling:

num-cpu 8
scheduler rr
quantum-cycles 5
batch-process-freq 1
min-ins 50
max-ins 50
delays-per-exec 0

For stress testing

num-cpu 8
scheduler fcfs
quantum-cycles 10
batch-process-freq 1
min-ins 100
max-ins 500
delays-per-exec 0
