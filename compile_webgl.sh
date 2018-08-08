ninja
emcc tekari.bc -o tekari.html -O3 -s WASM=1 -s USE_GLFW=3 -s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=0 --preload-file data_samples/
