# TenSure: Sparse Tensor Compiler Fuzzer

TenSure is a modular fuzzing framework designed to uncover correctness bugs in Sparse Tensor Compilers (STCs). It generates randomized sparse tensor kernels, executes them across compiler backends, and detects behavioral inconsistencies. TenSure is backend-agnostic and supports dynamic loading of multiple STC implementations.

This document provides a complete guide on building TenSure, running the fuzzer safely, and integrating additional compiler backends.

---

## 1. Building TenSure
Compile TenSure using CMake:
```bash
mkdir build && cd build
cmake .. -DBUILD_TACO=ON
make -j$(nproc)
```
Enabling `BUILD_TACO=ON` builds the TACO backend, which is included as a reference implementation.

---

## 2. Running the Fuzzer

TenSure is compute-intensive. To avoid system crashes or runaway memory usage, it is recommended to run the fuzzer:
- inside a Linux __cgroup__ (to isolate CPU and memory usage)
- inside a __tmux__ session (so execution continues even if SSH disconnects)

### 2.1 Set Up a Cgroup (One-Time Setup)
Create the cgroup:
```bash
sudo cgcreate -g memory,cpu:kfuzz
```

Limit memory usage:
```bash
sudo cgset -r memory.max=24G kfuzz
```

If TenSure exceeds this memory cap, only the fuzzer process is terminated—your machine remains stable.

Limit CPU usage:
```bash
sudo cgset -r cpu.max="800000 1000000" kfuzz
```
This configuration restricts TenSure to ~80% of a single CPU core.

---

### 2.2 Run the Fuzzer in tmux

Start a new session:
```bash
tmux new -s tensure
```

Execute TenSure inside the cgroup:
```bash
sudo cgexec -g memory,cpu:kfuzz ./TenSure --backend ./libtaco_wrapper.so
```

Detach while leaving the fuzzer running: <br>
`Ctrl+b`, then `d`

Reattach later:
```bash
tmux attach -t tensure
```

Useful tmux commands:
```bash
tmux ls
tmux kill-session -t tensure
```

All execution logs—including crashes, mismatches, and progress—are written to fuzzer.log.

---

## 3. Integrating New Compiler Backends

TenSure is built to support multiple Sparse Tensor Compilers. Backend implementations are isolated from the core fuzzing engine and are dynamically loaded at runtime.

This section describes how to generate the backend template, implement the required hooks, and compile TenSure with your backend.

### 3.1 Overview

- Each backend is implemented as a standalone module.
- TenSure generates sparse tensor kernels in JSON format.
- Each backend must:
    - Generate STC-specific code from the kernel specification.
    - Execute the generated program.
    - Provide the output in a standardized sparse tensor format.
    - Implement result comparison against the reference backend.

TACO’s backend implementation serves as the canonical example.

### 3.2 Create a Backend Template

Generate the scaffolding for your backend using:
```bash
python3 generate_module.py <backend_name>
```
This creates:
- A minimal CMake entry for the backend <br> (You must manually extend it to link compiler dependencies—see TACO’s CMakeLists as a reference.)
- Four C++ source files and their corresponding headers:
    - \<backend>_backend.cpp — defines the entry points.
    - Three additional files implementing:
        - kernel generation
        - kernel execution
        - result comparison

---

### 3.3 Backend Entry Points

Each backend must implement the following functions inside \<backend>_backend.cpp:

1. `generate_kernel`
- Takes a randomized JSON kernel specification as input.
- Produces a runnable STC-specific program.
- The generated program should write its output tensor to a file—this is essential for later comparison.

2. `execute_kernel`
- Executes the program produced by generate_kernel.
- Ensures that the output is written in the expected sparse format.

3. `compare_results`
- Compares the reference backend’s output with the mutated backend’s output.
- Reports discrepancies as potential compiler bugs.

Backends using a COO-like representation can reuse TenSure’s utility comparison functions.

__Required Output Format__ <br>
For reuse of utilities, backends should emit tensors in the following plain-text forms:

Matrices:
```bash
row col value
row col value
...
```

3-order tensors:
```bash
i j k value
i j k value
...
```

Refer to TACO’s backend implementation for the complete expected behavior.

## 4. Building TenSure with Your Backend

Compile TenSure with backend support enabled:
```bash
mkdir build && cd build
cmake .. -DBUILD_<BACKEND_NAME_IN_UPPERCASE>=ON
make -j$(nproc)
```
This produces:
- The main TenSure fuzzer executable
- A backend-specific shared library: `lib<backend_name>_wrapper.so`

---

## 5. Running the Fuzzer with a Custom Backend

Launch TenSure using your backend:

```bash
./TenSure --backend ./lib<backend_name>_wrapper.so
```

The fuzzer will:
1. Load the backend dynamically.
2. Generate and mutate kernel specifications.
3. Compile and execute kernels.
4. Compare results.
5. Log all findings to fuzzer.log and create bug directories.

## 6. Final Notes

TACO’s implementation is the recommended reference for backend authors.

The fuzzer is highly parallelizable—running inside a cgroup is strongly advised.

A backend is considered correct only if its output matches the reference compiler across all generated kernels.

---

The following is a reference for JSON specification of the randomly generated kernel by TenSure.

```json
{
    "computations": [
        {
            "expression": "A(i,l,m) = B(j,l,m,k,i) * C(i,n,j,k,l) * D(j,n,l,m,k) * E(n,l,j,k,i)"
        }
    ],
    "tensors": [
        {
            "dataFile": "-",
            "idxs": [
                105,
                108,
                109
            ],
            "name": "A",
            "shape": [
                5,
                6,
                5
            ],
            "storageFormat": [
                "Dense",
                "Sparse",
                "Dense"
            ],
            "str_repr": "A(i,l,m)"
        },
        {
            "dataFile": "fuzz_output/corpus/iter_0_20251121-142129/data/B.tns",
            "idxs": [
                106,
                108,
                109,
                107,
                105
            ],
            "name": "B",
            "shape": [
                4,
                6,
                5,
                6,
                5
            ],
            "storageFormat": [
                "Sparse",
                "Dense",
                "Dense",
                "Dense",
                "Sparse"
            ],
            "str_repr": "B(j,l,m,k,i)"
        },
        {
            "dataFile": "fuzz_output/corpus/iter_0_20251121-142129/data/C.tns",
            "idxs": [
                105,
                110,
                106,
                107,
                108
            ],
            "name": "C",
            "shape": [
                5,
                3,
                4,
                6,
                6
            ],
            "storageFormat": [
                "Sparse",
                "Sparse",
                "Sparse",
                "Dense",
                "Dense"
            ],
            "str_repr": "C(i,n,j,k,l)"
        },
        {
            "dataFile": "fuzz_output/corpus/iter_0_20251121-142129/data/D.tns",
            "idxs": [
                106,
                110,
                108,
                109,
                107
            ],
            "name": "D",
            "shape": [
                4,
                3,
                6,
                5,
                6
            ],
            "storageFormat": [
                "Sparse",
                "Sparse",
                "Dense",
                "Dense",
                "Dense"
            ],
            "str_repr": "D(j,n,l,m,k)"
        },
        {
            "dataFile": "fuzz_output/corpus/iter_0_20251121-142129/data/E.tns",
            "idxs": [
                110,
                108,
                106,
                107,
                105
            ],
            "name": "E",
            "shape": [
                3,
                6,
                4,
                6,
                5
            ],
            "storageFormat": [
                "Sparse",
                "Sparse",
                "Dense",
                "Dense",
                "Sparse"
            ],
            "str_repr": "E(n,l,j,k,i)"
        }
    ]
}
```


---

/home/kabilan/Desktop/TenSure/build/fuzz_output/failures/wc/iter_1799_20251117-092421

/home/kabilan/Desktop/TenSure/build/fuzz_output/failures/wc/iter_8549_20251117-135741

/home/kabilan/Desktop/TenSure/build/fuzz_output/failures/wc/iter_3059_20251117-101549