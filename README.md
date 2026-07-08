# LessetG

A calculator. With a GUI. That graphs. 

Also, it has a small scripting language. And like, 64 functions. And variables. And macros. And a whole lotta jank.

<img src="/Assets/graph.png" height="75%" width="75%">

To compile:

- Install the Boost C++ library https://www.boost.org/

- Get a copy of NotoSansMathRegular

- Check https://github.com/ocornut/imgui/blob/master/docs/FONTS.md#loading-font-data-embedded-in-source-code 

- Use https://github.com/ocornut/imgui/blob/master/misc/fonts/binary_to_compressed_c.cpp to turn the font into a C file.

- Rename that file to NotoSansMathRegular.hpp and make sure the names of the variables therein are "NotoSansMathRegular_compressed_size" and "NotoSansMathRegular_compressed_data"

- Set paths on lines 19 and 30 of CMakeLists.txt (to GLFW and ImGui respectively).

- Follow instructions at the top of CMakeLists.txt

- You may need to either change some "path" includes to \<path\> includes or include the relevant files in the project directory in case they aren't found.

- This probably only works on linux. The probably is for it working on linux.

This readme currently sucks. In case you see this line, feel free to tell me to make it better, because I forgot or haven't gotten to it yet.

No coding agents or LLMs or similar were used for this. In case this line disappears, I have succumbed to the dark side.
