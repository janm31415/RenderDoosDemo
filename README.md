# RenderDoosDemo

Example applications using [RenderDoos](https://github.com/janm31415/RenderDoos), which provides a uniform API for compute shaders in OpenGL and Metal.


### Building
The code can be build using CMake. The examples use [FLTK](https://www.fltk.org) for the UI, which is delivered with the code, so no extra repositories need to be downloaded or installed. Note however that RenderDoos and FLTK are Git submodules, so don't forget to call

    git submodule update --init
    
before you run CMake.
