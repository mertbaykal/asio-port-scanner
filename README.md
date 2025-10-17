# asio-port-scanner

TCP connect port scanner built with Boost.Asio — lightweight, cross-platform C++ (optimized for macOS M1/M2).
**For authorized testing only.** Use this tool only against systems you own or have explicit permission to test.

---

## Features

* TCP **connect** based scanning (no raw sockets, no root required)
* Async + thread-pool friendly (Boost.Asio)
* Port ranges and comma-separated port lists (e.g. `1-1024`, `22,80,443`)
* Configurable concurrency (threads) and per-port timeout (ms)
* Simple banner-grabbing attempt for common services (HTTP HEAD)
* macOS (Apple Silicon / ARM64) friendly CMake configuration

---

## Quick links

* Repository: `asio-port-scanner`
* Language: C++17 (Boost.Asio)
* Build system: CMake

---

## Requirements

* macOS (works on Intel too; instructions below target Apple Silicon)
* Homebrew (recommended)
* boost (Homebrew: `brew install boost`)
* cmake
* A C++17-capable compiler (Apple Clang shipped with Xcode Command Line Tools)

---

## Installation (macOS M1/M2)

```bash
# 1) Install dependencies (if needed)
brew update
brew install boost cmake

# 2) Build
git clone <repo-url> asio-port-scanner
cd asio-port-scanner
mkdir build
cd build

# If CMake cannot find Homebrew packages, pass the prefix:
cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew

cmake --build . --config Release
```

If you prefer CLion:

1. Open CLion → Open → choose project root (where `CMakeLists.txt` is).
2. If Boost is not detected automatically, add to CMake options:

```
-DCMAKE_PREFIX_PATH=/opt/homebrew
```

3. Build/run via CLion UI. Add program arguments in the Run Configuration.

---

## Usage

```
./scanner <target> <ports> [threads] [timeout_ms]
```

* `target` — hostname or IP (e.g. `example.com` or `192.168.1.10`)
* `ports` — port specifier: `1-1024` or `22,80,443` or mixed `20-25,80`
* `threads` — optional, number of worker threads (default: `100`)
* `timeout_ms` — optional, per-port timeout in milliseconds (default: `300`)

Examples:

```bash
# scan a few well-known ports
./scanner 127.0.0.1 22,80,443 50 400

# scan 1-1024 with 200 threads and 300 ms timeout
./scanner example.com 1-1024 200 300
```

---

## Output

* Prints detected open ports to stdout in real-time:

```
[OPEN] 22  banner: SSH-2.0-OpenSSH_8.2p1...
[OPEN] 80  banner: HTTP/1.1 200 OK...
```

* At the end a short summary displays total open ports.

---

## Notes & Limitations

* This scanner uses **TCP connect** method (no SYN/stealth scan). That makes it portable and does not require root privileges, but it may be noisier than raw-packet scans.
* Banner grabbing is minimal (HTTP HEAD probe) — you can extend with protocol-specific probes (SSH, SMTP, FTP, etc.).
* Concurrency / high thread counts may overwhelm your local machine or the target. Use responsibly.
* Timeouts and thread limits are configurable; adjust to your environment.

---

## Security & Legal

**Do not** run this tool against networks/systems you do not have permission to test. Unauthorized scanning may be illegal and can trigger intrusion detection/mitigation. Always obtain explicit authorization and keep records of permission.

---

## Troubleshooting

* **CMake can't find Boost**: Make sure Homebrew install path is included:

  ```
  cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew
  ```

  Or set `CMAKE_PREFIX_PATH` in CLion CMake options.
* **Linker errors for Boost libraries**: Verify `brew info boost` and that `Boost_LIBRARIES` are visible to CMake. Ensure Xcode Command Line Tools are installed: `xcode-select --install`.
* **Performance**: Lower the `threads` parameter if the system becomes overloaded.

---

## Development ideas / TODO

* Improve banner grabbing with protocol-aware probes (SSH, SMTP HELO, FTP NOOP).
* Add output formats: JSON / CSV.
* Implement rate-limiting / jitter to be "network friendly".
* Add resume/retry capability and multi-target scanning.
* Add tests and CI (GitHub Actions) for Linux/macOS build checks.

---

## Contributing

Contributions welcome (bug fixes, improvements, tests).

1. Fork the repo
2. Create a feature branch (`git checkout -b feat/my-change`)
3. Commit your changes (`git commit -m "Add ..."`)
4. Open a Pull Request with a clear description

---

## License

This project is distributed under the **MIT License**. See `LICENSE` for details.

---

## Contact

If you find issues or have feature requests, open an issue in the repository. For direct questions, mention your GitHub username in the issue and I’ll respond.

---

**Reminder:** This tool is intended for educational and authorized testing only. Use responsibly.
