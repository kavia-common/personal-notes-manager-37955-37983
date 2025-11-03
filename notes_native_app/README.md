# Personal Notes - Native Qt6 App

A native desktop application for creating, editing, searching, and managing personal notes. Built with Qt6 and CMake. Uses local JSON storage (offline-first) and the "Ocean Professional" theme (modern, rounded corners, subtle shadows).

Features:
- Notes list with search (title and body) and sort (updated date, title)
- Create, edit, and delete notes (with confirmation)
- Local persistence in a JSON file under your AppData location
- Seed demo notes on first launch
- Light/Dark theme toggle in Settings
- Modern Ocean Professional styling:
  - Primary: #2563EB
  - Secondary/Success: #F59E0B
  - Error: #EF4444
  - Background: #f9fafb
  - Surface: #ffffff
  - Text: #111827

Project layout:
- include/mainApp.h — Public interfaces and widget declarations
- src/mainApp.cpp — Implementation (theme, storage, UI screens, app entry)
- CMakeLists.txt — Build configuration

Build and run (from container root):
1) mkdir -p build && cd build
2) cmake .. && cmake --build . -j
3) ./MainApp  (or use: cmake --build . --target run)

Tests:
- A basic smoke test is registered in CTest to start the application binary.
- For more comprehensive unit tests, add GoogleTest/QtTest in future work. TODOs are marked in code comments.

Local storage:
- Notes are saved in: QStandardPaths::AppDataLocation/notes.json
- Corrupted files are backed up as notes.json.corrupt on load, then reseeded.

Keyboard tips:
- Double-click a note to open it
- Use the "New Note" button to create

Limitations:
- No cloud sync. Fully offline/local.
- Tags are free-form text (comma-separated).

License: MIT
