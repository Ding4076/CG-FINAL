# external/ — third-party libraries (NOT committed; regenerated)

This directory holds physically-copied third-party libraries, matching the TA's
"from scratch" project structure. It is gitignored to avoid bloating the repo
(the same libs exist at the repo root as submodules/tracked dirs).

To (re)generate, run from the repo root:

```bash
git submodule update --init external/glfw external/glm
cp -r external/glad  projects/finalproject/external/
cp -r external/glfw  projects/finalproject/external/
cp -r external/glm   projects/finalproject/external/
cp -r external/imgui projects/finalproject/external/
cp -r external/stb   projects/finalproject/external/
mkdir -p projects/finalproject/external/miniaudio
# fetch miniaudio.h (~1MB) into projects/finalproject/external/miniaudio/  (Task 8)
```

tinyobjloader is INTENTIONALLY excluded — the OBJ loader is hand-written
(source/base/obj_loader.{h,cpp}), per the TA requirement.

When packaging the submission (zip), DO include this directory so the project
builds standalone for the grader.
