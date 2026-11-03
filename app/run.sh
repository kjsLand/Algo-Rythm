cd "$(dirname "$0")/.." || exit 1

cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug

# macOS: ./build/app/AlgoRythm_artefacts/Debug/Algo-Rythm.app/Contents/MacOS/Algo-Rythm
# Linux: ./build/app/AlgoRythm_artefacts/Debug/Algo-Rythm
