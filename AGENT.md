# AGENT.md

## Purpose

This repository contains the C++ volumetric ray tracer assignment for **Introduction to Computer Graphics and Visualization**.

Work should:

- Compile with CMake using C++17.
- Prefer native macOS builds first on this machine.
- Fall back to Linux via Orb only if native compilation fails because of platform-specific tooling or headers.
- Preserve program behavior and rendered output.
- Submit only `CMakeLists.txt` and `src/` to Themis.

Optimize for **clarity and correctness**, not cleverness or broad refactors.

## Critical Rules

Always:

- Work in the framework directory:

  ```sh
  cd icgv-volumetric-ray-tracer
  ```

- Configure and build with CMake:

  ```sh
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --parallel 10
  ```

- Run scenes from the framework root so relative volume paths resolve correctly:

  ```sh
  ./build/ray scenes/other/scene01.json
  ./build/ray scenes/05_intersection/1.json
  ```

- Use native macOS compilation first. This codebase is C++17 and does not appear to depend on Linux-only APIs.
- Use Orb only as a fallback if macOS tooling cannot compile the project.
- Keep changes scoped to the assignment task being solved.
- Prefer small, explicit, readable fixes over refactors.

Never:

- Change scene file formats, JSON attribute names, or constants used by parsing.
- Edit expected/reference `.png` files to “fix” results.
- Change program behavior unrelated to the current sub-assignment.
- Commit generated build products or rendered output unless explicitly requested.
- Run Themis inside Orb.

## C++ Style

### Naming

- Use the existing codebase style first.
- Existing classes use names like `Scene`, `Raytracer`, `DensityField`, `TransferFunction`, `Sphere`, `Hit`, and `Triple`.
- Existing member functions use lower camel case or mixed style, for example `shadeHit`, `intersectObjects`, `densityAt`, and `shared_from_this`.
- Keep new names descriptive and consistent with nearby code.
- Short names are acceptable for established graphics/math terms, for example `t`, `N`, `L`, `V`, `O`, `D`, `aa`, and `bb`, when they directly match assignment formulas.

### Implementation Style

- Prefer the simplest correct implementation that matches the assignment text.
- Keep control flow direct and explicit.
- Avoid clever one-liners in geometry, shading, and compositing code.
- Use existing helpers and operators in `triple.h` instead of duplicating vector math.
- Use standard C++17 and the project’s existing includes.
- Avoid unnecessary abstractions unless they clearly reduce repeated assignment logic.
- Preserve floating-point behavior carefully; small numerical changes can affect rendered images.
- Use `scene.h` epsilon where the assignment specifies it.

## Repository Layout

```text
icgv-assignments/
  AGENT.md
  icgv-assignment.md
  icgv-volumetric-ray-tracer/
    CMakeLists.txt
    build.sh
    README.md
    src/
    scenes/
  volume_data/
```

Important files:

- [Assignment](./icgv-assignment.md)
- [Framework README](./icgv-volumetric-ray-tracer/README.md)
- [CMakeLists.txt](./icgv-volumetric-ray-tracer/CMakeLists.txt)
- [scene.cpp](./icgv-volumetric-ray-tracer/src/scene.cpp)
- [scene.h](./icgv-volumetric-ray-tracer/src/scene.h)
- [sphere.cpp](./icgv-volumetric-ray-tracer/src/objects/shapes/sphere.cpp)
- [volume.cpp](./icgv-volumetric-ray-tracer/src/volumes/volume.cpp)
- [densityfield.cpp](./icgv-volumetric-ray-tracer/src/volumes/densityfield.cpp)
- [triple.h](./icgv-volumetric-ray-tracer/src/triple.h)

The links above are relative Markdown links so Zed can open them from the workspace.

## Native macOS Build

Use this path first on the M3 Pro MacBook Pro.

```sh
cd icgv-volumetric-ray-tracer
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 10
```

The project can use OpenMP for faster rendering. On macOS, OpenMP may require Homebrew `libomp` depending on the compiler. The CMake configuration allows a serial build if OpenMP is unavailable, which should preserve rendered output while being slower.

The included script is also valid:

```sh
cd icgv-volumetric-ray-tracer
./build.sh
```

If new `.cpp` files are added, re-run CMake before building:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

## OrbStack Fallback

Use Orb only if native macOS compilation fails for a real platform reason.

```sh
cd icgv-volumetric-ray-tracer
orb cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
orb cmake --build build --parallel 10
```

Orb commands require elevated permissions in this environment. Request escalation before running them.

Do **not** use Orb for Themis commands.

## Running the Ray Tracer

Run from `icgv-volumetric-ray-tracer/`, not from `build/`, for scenes that refer to `../volume_data`.

Examples:

```sh
cd icgv-volumetric-ray-tracer

# Default output path next to the scene JSON.
./build/ray scenes/other/scene01.json

# Explicit output path.
./build/ray scenes/05_intersection/1.json /tmp/icgv-05-intersection.png
```

Useful smoke-test scenes:

```sh
./build/ray scenes/01_ambient/1.json
./build/ray scenes/02_diffuse/1.json
./build/ray scenes/03_multilights/1.json
./build/ray scenes/04_shadows/1.json
./build/ray scenes/05_intersection/1.json
./build/ray scenes/06_compositing/1.json
./build/ray scenes/07_sampling/1.json
./build/ray scenes/08_diffuse/1.json
./build/ray scenes/09_shadows/1.json
./build/ray scenes/10_integration/1.json
```

## Themis Configuration

Base course URL:

```text
THEMIS_REPO_BASE_URL=https://themis.housing.rug.nl/course/2025-2026/icgv
```

Use Themis on macOS, not via Orb.

Discovery:

```sh
themis list \
  --discover \
  --root-url "$THEMIS_REPO_BASE_URL" \
  --discover-depth 8
```

JSON discovery:

```sh
themis list \
  --discover \
  --root-url "$THEMIS_REPO_BASE_URL" \
  --discover-depth 8 \
  --json
```

Note: `themis` accesses the internet and requires elevated permissions in this environment.

## Submission Packaging

Each sub-assignment expects an archive containing only:

- `CMakeLists.txt`
- `src/`

Create a submission archive from `icgv-volumetric-ray-tracer/`:

```sh
cd icgv-volumetric-ray-tracer
tar -czf ../icgv-submission.tar.gz CMakeLists.txt src
```

Do not include:

- `build/`
- `scenes/`
- generated `.png` files
- `volume_data/`
- editor metadata

## Testing and Verification

There is no strict text `diff` test loop for this project. Verification is primarily:

- Build successfully.
- Render the scene for the current sub-assignment.
- Compare the generated image visually against the provided reference `.png`.
- Submit to the matching Themis sub-assignment and confirm it passes.

When changing math-heavy code, test at least the current scene and one earlier scene to avoid regressions. For example, after shadow changes, run the shadow scene and a diffuse scene.

## Agent Behavior

When modifying code in this repo:

- Read the assignment section before implementing the corresponding task.
- Inspect the existing class and helper APIs before adding new code.
- Build with CMake after changes.
- Run at least one relevant scene when possible.
- Keep any build-system changes behavior-preserving.
- Prefer native macOS builds; use Orb only if native build is blocked.
- Do not modify unrelated files or generated outputs.
- Mention any verification that could not be run.

## Teaching and Scaffolding Mode

When the user explicitly asks to **learn** (for example “teach me”, “explain this”, “walk me through it”):

- Optimize for learning over speed.
- Explain step by step in short paragraphs.
- Connect the code to the formulas in `icgv-assignment.md`.
- Use small, focused helper functions only when they clarify the task.
- Leave important assignment logic for the user when they ask for scaffolding rather than a full solution.

In scaffolding mode, **do not**:

- Implement whole sub-assignments unless the user asks.
- Hide the key graphics/math idea inside a large helper abstraction.
- Rewrite broad parts of the framework.

Instead:

- Point to the exact files and functions involved.
- Provide a small compilable skeleton or local TODOs.
- Explain the expected inputs, outputs, and edge cases.

## Theory and Discussion Questions

If you write theory or discussion answers, keep them straight to the point using basic English and simple connectors. Keep them at an undergraduate level. Prefer paragraph form and do not overuse bullet points. Do not extend knowledge outside of lectures, the assignment, or the provided framework unless asked.

Write like a STEM student, not a writer. Grammar does not need to be perfect, slight imprecision is fine and expected. Sentences can be a bit run-on or informal. Avoid overly polished or essay-like language. Do not use complex vocabulary when a simpler word works. Spelling mistakes on longer or less common words are acceptable and natural. The tone should feel like someone who understands the material well but is not precious about how they write it down.
