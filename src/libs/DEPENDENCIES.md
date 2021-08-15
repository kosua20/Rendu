# Dependencies

Dependencies are stored in `src/libs`.  All dependencies are built along with the engine library. glfw3 and nfd require specific build phases that are defined in custom premake files in the corresponding directories. All other dependencies are directly integrated in the engine. To update the dependencies, follow the rules below:

* **glfw**: download source from https://github.com/glfw/glfw/releases, copy `src` and `include` to local `glfw` directory (`premake` file is provided).
* **glm**: download source from https://github.com/g-truc/glm/releases, copy content of `src` to local `glm` directory.
* **imgui**: download source from https://github.com/ocornut/imgui/releases, copy all `*.{h, cpp}` files from the root *except* `imconfig.h` to local `imgui` directory, along with `backends/imgui_impl_{vulkan, glfw}.{h, cpp}`. Apply `imgui/vulkan_patch.diff`.
* **miniz**: download source from https://github.com/richgel999/miniz/releases, copy `miniz.{h,c}` to local `miniz` directory.
* **nfd**: download source from https://github.com/mlabbe/nativefiledialog, copy content of `src` and `src/include` to local `nfd` directory (`premake` file is provided).
* **stb_image**: download source from https://github.com/nothings/stb, copy `stb_image.h` and `stb_image_write.h` to local `stb_image` directory.
* **tinydir**: download source from https://github.com/cxong/tinydir, copy `tinydir.h` to local `tinydir` directory.
* **tinyexr**: download source from https://github.com/syoyo/tinyexr, copy `tinyexr.h` to local `tinyexr` directory.
* **vma**: download source from https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/, copy `include/vk_mem_alloc.h` to local `vma` directory.
* **volk**: download source from https://github.com/zeux/volk, copy `volk.{h,c}` to local `volk` directory.
* **xxhash**: download source from https://github.com/Cyan4973/xxHash, copy `xxhash.h` to local `xxhash` directory.