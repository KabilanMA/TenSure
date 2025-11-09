## TenSure - Sparse Tensor Compiler Fuzzer

Build Process

```bash
mkdir build && cd build
cmake .. -DBUILD_TACO=ON
make -j$(nproc)
./TenSure --backend ./libtaco_wrapper.so
```