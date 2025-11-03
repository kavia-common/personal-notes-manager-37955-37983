# Tests Scaffold

Planned tests (QtTest or GTest integration to be added later):

- NotesStorage:
  - load() seeds on first run
  - upsert() creates and updates notes, updates timestamps
  - remove() deletes by id
  - list() filters by search and sorts by keys

- UI (QtTest GUI):
  - List displays seeded notes
  - Search filters results
  - Editor saves and delete confirmation works
  - Theme toggle updates stylesheet

To integrate QtTest:
- Add `find_package(Qt6 REQUIRED COMPONENTS Test)` in CMakeLists.txt
- Create a `tests/` executable linked with Qt6::Test
- Register with `add_test`

For now, a smoke test exists via `add_test(NAME MainAppTest COMMAND MainApp)`.
