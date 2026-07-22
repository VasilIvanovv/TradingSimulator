Frontend stack: Tauri v2 + React + TypeScript

The frontend targets three platforms in this order: Windows desktop first, then web browser, then Android. The stack was chosen to serve all three from a single React codebase.

---

Why Tauri v2

Tauri v2 wraps a web UI in a native application shell. It supports Windows, macOS, Linux, Android, and iOS from the same codebase. The UI is rendered in the operating system's built-in WebView (Edge WebView2 on Windows, WebKit on macOS/Linux), which means no bundled browser like Electron — the resulting installer is dramatically smaller and memory usage is lower.

The developer already has Tauri v2 experience from the Matchmaker project, so no new toolchain needs to be learned.

Alternatives considered:
- Electron: heavier (bundles Chromium), larger installer, higher memory use. No meaningful advantage over Tauri for this use case.
- Pure web app: valid for the web and Android targets but does not produce a native Windows installer. Starting with Tauri now avoids restructuring later.

---

Why React + TypeScript

React is the UI library used in Matchmaker, so it is already familiar. TypeScript is used instead of plain JavaScript because it catches type errors at compile time, provides autocomplete for the API response shapes, and makes refactoring safer as the UI grows.

Vite is the build tool (standard for Tauri projects) — it compiles TypeScript, bundles assets, and serves the dev server during development.

---

How this differs from Matchmaker

In Matchmaker, the backend logic (ELO calculation, data persistence) is written in Rust and runs inside the Tauri process. The React frontend communicates with it via Tauri's invoke mechanism — a typed IPC bridge with no HTTP involved:

    React → invoke('calculate_elo', { players }) → Rust (inside Tauri)

TradingSimulator is structured differently. The backend is C++ and runs as a completely separate process with its own HTTP API. Tauri's role here is purely a window shell — it provides the native window frame, handles packaging and installation, and can optionally spawn the C++ backend as a sidecar process on startup.

The React frontend communicates with the C++ backend using ordinary fetch() calls, exactly as it would in a web app:

    React → fetch('http://localhost:8080/account') → C++ backend

No Rust bridge code, no invoke commands, and almost no Rust needs to be written at all for this project. The Tauri layer is thinner here than in Matchmaker.

---

The sidecar pattern for Windows

On Windows, the C++ trading_simulator.exe is bundled with the Tauri installer as a sidecar — a companion executable that Tauri spawns automatically when the app starts and terminates when the app closes. From the user's perspective, opening the app starts everything; there is no separate backend process to manage.

For the web and Android targets, the C++ backend runs on a cloud server. The frontend connects to it via a configurable base URL (an environment variable set at build time) rather than hardcoded localhost. This means the same React codebase works for all three targets — it simply points at different URLs depending on how it was built.

---

Platform rollout plan

1. Windows — Tauri v2 desktop app, C++ backend as a local sidecar.
2. Web browser — same React code served as a static web app, C++ backend deployed to the cloud.
3. Android — same React code wrapped in Tauri Mobile, C++ backend on the cloud.

No UI code needs to be rewritten between platforms. Responsive layout is designed from the start to accommodate both desktop and mobile screen sizes.
