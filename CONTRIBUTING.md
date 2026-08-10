# Contributing to Strata Studio

Thank you for your interest in contributing to **Strata Studio**! We welcome contributions from C++ developers, DSP engineers, audio UI designers, and AI agent developers.

---

## Architectural Rules & Guidelines

Strata Studio enforces a strict **8-Layer Architecture** to ensure real-time safety, maintainability, and deterministic performance:

1. **Directional Dependency**: Layer $N$ may only depend on layers below it ($N-1, N-2, \dots$). No upward imports are permitted.
2. **Real-Time (RT) Thread Safety**:
   - **NO Dynamic Allocations** (`malloc`, `new`, `std::vector::push_back`) in audio processing nodes or engine loops.
   - **NO Locking / Mutexes**: Use lock-free SPSC ring buffers and atomic arrays from Layer 2.
   - **NO Disk / File / Network I/O** or OS system calls on the audio callback thread.
3. **Presentation & Agentic Decoupling**: Layer 7 (Presentation) and Layer 8 (Agentic IPC) interact exclusively with Layer 6 and below via the **Middle Bridge** (`src/Middle Bridge/` under `namespace bridge`).
4. **C++20 Standard**: All code must be written in Modern C++20, utilizing concept-constrained templates and POD boundary primitives.

---

## Development Workflow

### 1. Build and Run Tests
Before submitting a PR, verify that all unit tests pass:
```bash
./scripts/install_dependencies.sh
./scripts/build.sh release --test
```

### 2. Code Style & Formatting
- Follow modern standard C++ naming conventions.
- Keep header includes clean and scoped to the lowest required layer.
- Preserve explicit 1-to-1 planar port routing in DSP nodes.

### 3. Submitting Pull Requests
- Open a PR against the `main` branch.
- Include a summary of your changes, reference any related issues, and state which architectural layer your changes affect.
