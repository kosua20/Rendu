# Dependencies

Dependencies are stored in `src/libs`.  All dependencies are built along with the engine library. glfw3 and gl3w require specific build phases that are defined in custom premake files in the corresponding directories. All other dependencies are directly integrated in the engine. To update the dependencies, follow the rules below:

* **gl3w**: clone https://github.com/skaslev/gl3w, run `python gl3w_gen.py`, copy output files in local `gl3w` directory (with no subdirectories), update include in headers and rename `gl3w.c` to `gl3w.cpp` (because it's lacking an `extern "C" {...}` block).
* **glfw**: download source from https://github.com/glfw/glfw/releases, copy `src` and `include` to local `glfw` directory (`premake` file is provided).
* **glm**: download source from https://github.com/g-truc/glm/releases, copy content of `src` to local `glm` directory.
* **imgui**: download source from https://github.com/ocornut/imgui/releases, copy all `*.{h, cpp}` files from the root *except* `imconfig.h` to local `imgui` directory, along with `examples/imgui_impl_{opengl3, glfw}.{h, cpp}`.
* **miniz**: download source from https://github.com/richgel999/miniz/releases, copy `miniz.{h,c}` to local `miniz` directory.
* **nfd**: download source from https://github.com/mlabbe/nativefiledialog, copy content of `src` and `src/include` to local `nfd` directory (`premake` file is provided).
* **stb_image**: download source from https://github.com/nothings/stb, copy `stb_image.h` and `stb_image_write.h` to local `stb_image` directory.
* **tinydir**: download source from https://github.com/cxong/tinydir, copy `tinydir.h` to local `tinydir` directory.
* **tinyexr**: download source from https://github.com/syoyo/tinyexr, copy `tinyexr.h ` to local `tinyexr` directory.