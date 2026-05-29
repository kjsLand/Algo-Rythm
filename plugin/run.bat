# build.sh
#!/bin/bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug

# ./build/JUCEProject_artefacts/Debug/JUCE\ Project.exe