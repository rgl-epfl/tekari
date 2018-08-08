ninja
emcc -O3 tekari.bc -o tekari.js -s WASM=1 -s USE_GLFW=3 -s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=0
