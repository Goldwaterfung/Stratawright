# Stratawright FAQ: Open-Source AI DAW & Agentic Music Production

> **Frequently Asked Questions about Stratawright** — The open-source, C++20 agentic Digital Audio Workstation (DAW) natively controlled by AI agents (Claude Code, Cursor, Codex, Hermes, Gemini).

---

## 1. Core Concepts & Definitions

### What is Stratawright?
**Stratawright** is an open-source, high-performance C++20 Digital Audio Workstation (DAW) designed specifically for **agentic music production**. It features a dedicated IPC server daemon and command-line interface (`daw-cli`) that allows AI agents—such as Claude Code, Cursor, Codex, Hermes, or Gemini—to natively control recording, track creation, mixing, gain staging, routing, and VST3/AU plugin parameters using natural language prompts.

### What is an Agentic DAW?
An **Agentic DAW** is a Digital Audio Workstation engineered from the ground up to let autonomous AI agents interact directly with the DAW's session model and audio processing graph. Unlike traditional DAWs that rely exclusively on manual mouse clicks, key commands, and complex graphic user interfaces (GUIs), an agentic DAW exposes deterministic IPC (Inter-Process Communication) and structured JSON APIs so AI agents can execute multi-step session edits, stem grouping, automated gain staging, and effect routing in parallel.

### How does Stratawright compare to traditional DAWs like Ableton Live, Logic Pro, or Pro Tools?
Stratawright complements traditional DAWs by removing repetitive session prep and administrative labor:

| Feature | Traditional DAWs (Ableton, Logic, Pro Tools) | Stratawright (Agentic DAW) |
| :--- | :--- | :--- |
| **Control Interface** | Manual keyboard & mouse, complex GUI menus | Natural language AI prompts & deterministic CLI (`daw-cli`) |
| **Session Setup & Prep** | Manual (15–45 mins per session to label, color, route) | Instant AI automation (< 3 seconds via natural language) |
| **Gain Staging** | Manual adjustment of individual clip levels and faders | Automatic multi-track normalization to -18 dBFS sweet spot |
| **Stem Grouping** | Manual track selection and sub-mix bus routing | Semantic understanding of track names (`GTR_L` → Guitar Bus) |
| **Audio Thread Safety** | Plugin GUI / main thread blocking risks | 8-layer decoupled C++20 engine with lock-free RT communication |
| **Extensibility** | Closed script environments or basic OSC/MIDI APIs | Native LLM agent IPC daemon with structured JSON output |

---

## 2. Recording, MIDI & Session Setup

### How does Stratawright help capture song ideas faster without technical DAW setup?
Traditional DAWs force songwriters to stop their creative flow to manually create tracks, select audio inputs, assign colors, and configure I/O routing before recording a single note. In **Stratawright**, you tell your AI assistant what you want to record (e.g., *"Set up an acoustic guitar track and a lead vocal track with reverb"*). The AI agent creates, configures, and arms the tracks in under 3 seconds so you stay 100% in your creative zone.

### Do I need to learn complex keyboard shortcuts or software menus?
No. Stratawright is designed to eliminate the "shortcut tax." Every DAW operation—from transport controls and track creation to plugin hosting—can be triggered using plain natural language prompts through connected AI assistants (Claude Code, Cursor, Gemini, etc.) or standard CLI commands (`daw-cli`).

### Can I ask my AI assistant to set up a full recording template with one prompt?
Yes. You can prompt your AI agent with broad requests like: *"Build me a pop production template with 4 vocal tracks, stereo acoustic guitars, bass DI, and a drum bus."* The AI agent communicates with the DAW's IPC daemon to instantiate all tracks, assign semantic colors, load default channel strips, and group sub-mix buses in a single pass.

### Can I edit MIDI events and pattern clips using natural language prompts?
Yes. Through `daw-cli`, AI agents can programmatically insert, quantize, transpose, and adjust velocity values for MIDI notes and sequence clip regions on instrument tracks.

---

## 3. Mixing, Gain Staging & Plugins

### How does Stratawright eliminate tedious session prep and housekeeping before a mix?
Mix engineers routinely spend 30–60 minutes per session renaming messy tracks, color-coding stems, normalizing levels, creating sub-mix buses, and loading vocal chains. Stratawright automates this entire janitorial phase. A single prompt (e.g., *"Prep this session for mixing"*) triggers automatic track organization, gain staging, bus routing, and plugin chain instantiation.

### How does automatic gain staging work, and why does it target -18 dBFS nominal RMS?
When prompted to gain stage a session, the AI agent inspects multi-track peak and RMS signal levels using non-destructive audio intelligence. It calculates exact gain offsets to bring average track levels to **-18 dBFS nominal RMS** (with a -6 dBFS peak ceiling). This places your audio into the optimal analog-modeled sweet spot for third-party VST3/AU plugins and prevents digital clipping on mix buses.

### Can Stratawright understand track naming conventions and automatically group stems into sub-mix buses?
Yes. Using semantic track comprehension, AI agents analyze track labels like `GTR_L_57`, `Kick_In_57`, `Vox_Lead_Harm_01`, and `Bass_DI`. The agent automatically groups related tracks into dedicated sub-mix buses (e.g. `Drums Bus`, `Guitars Bus`, `Vocal Bus`), assigns matching color palettes, and configures planar output ports without requiring manual drag-and-drop routing.

### Can my AI agent scan, batch-load, and tweak third-party VST3, AU, and CLAP plugins?
Yes. Stratawright features a native cross-platform plugin host layer. AI agents can execute `daw-cli plugin scan` to index your installed VST3, Audio Units (AU on macOS), and CLAP plugins, instantiate effect chains across track slots, and automate parameter values using normalized float controls.

### How are plugin delay compensation (PDC) and latency managed?
Stratawright automatically calculates plugin latencies across the DSP processing graph and applies sample-accurate delay offsets to faster signal paths, keeping all multi-track audio and MIDI signals in strict phase alignment.

### Can I export stems and deliverables automatically?
Yes. Users can prompt their AI assistant to bounce and export project deliverables (e.g., *"Export clean stems, instrumental mix, and main master WAV"*). The agent communicates with `daw-cli` to handle offline rendering, track muting/soloing, and file tagging automatically.

---

## 4. Architecture & IPC Integration

### How do I connect my AI agent or LLM CLI (Claude Code, Cursor, Gemini, Custom MCP) to Stratawright?
Stratawright runs a high-performance IPC server daemon listening on a local UNIX domain socket (`/tmp/daw_session.sock`). AI agents interact with the DAW via the lightweight `daw-cli` binary or custom socket client implementations. Programmatic queries support `--format json` output for easy schema parsing by LLMs.

### How does Stratawright guarantee real-time audio safety and zero dropouts during AI agent mutations?
Stratawright uses a strict **8-Layer Decoupled C++20 Architecture**. Agent commands received by the IPC server daemon mutate state in the Middle Bridge controller layer. Communication between the Middle Bridge and the core audio processing engine uses **lock-free single-producer single-consumer (SPSC) ring buffers** and atomic state arrays. The audio execution thread never allocates dynamic memory (`malloc`/`new`), executes system calls, or blocks on lock mutexes, guaranteeing zero buffer underruns (glitches/clicks).

### Why is `daw-cli` built on a UNIX domain socket daemon rather than a blocking GUI script engine?
Traditional GUI scripting (like AppleScript or UI automation macros) is slow, fragile, and prone to UI thread blocks. `daw-cli` communicates directly with an asynchronous IPC socket daemon operating at the application layer. This provides deterministic execution speed (< 5 ms latency per command), atomic multi-command batching, and complete decoupling from the GUI frame rate.

---

## 5. Offline Execution, Privacy & Hardware Support

### Does Stratawright require an internet connection to function?
No. Stratawright runs 100% locally. The C++ core engine and `daw-cli` IPC daemon operate on your local machine (`/tmp/daw_session.sock`), allowing complete offline usage. You can pair it with local open-weights LLMs (such as Ollama, Llama.cpp, or Hermes) for a private, offline AI DAW environment.

### What audio drivers and operating systems are supported?
Stratawright is built in cross-platform **Modern C++20**:
- **macOS**: Native CoreAudio driver HAL with low-latency Apple Silicon (M1/M2/M3/M4) optimization.
- **Linux**: ALSA, JACK, and PipeWire support.
- **Windows**: Low-latency ASIO and WASAPI support.

### How does Stratawright handle CPU load during 100+ track sessions?
The engine features dynamic **graph silence propagation** to short-circuit idle tracks, 32-bit floating-point planar buffers optimized for **SIMD vectorization (AVX2/NEON)**, and multi-threaded parallel execution across CPU cores.

---

## 6. Developer & Open Source Community

### Is Stratawright open-source?
Yes. Stratawright is released as an open-source project. Developers and audio engineers are encouraged to contribute to the core C++20 engine, build custom agent skills, or integrate new IPC capabilities.

### How do I build Stratawright from source?
Building Stratawright requires CMake, a C++20 compliant compiler (Clang/GCC/MSVC), and vcpkg:
```bash
# 1. Install build tools and bootstrap vcpkg
./scripts/install_dependencies.sh

# 2. Build release binary
./scripts/build.sh release

# 3. Launch application
./build/release/bin/agent_based_daw
```
For 1-prompt automated setup, open your AI agent in the repository directory and ask: `"Build and package Stratawright for me"`.

---

## 7. Resources & Links

- **Main Repository**: [Stratawright GitHub Repository](https://github.com/Goldwaterfung/Stratawright)
- **Agent Skill Documentation**: [skills/daw-cli/SKILL.md](skills/daw-cli/SKILL.md)
- **Architecture Specification**: [ARCHITECTURE.md](ARCHITECTURE.md)
- **LLM Agent Reference Index**: [llms.txt](llms.txt) | [llms-full.txt](llms-full.txt)
