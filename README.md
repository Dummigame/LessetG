# LessetG

A calculator. With a GUI. That graphs. 

Also, it has a small scripting language. And like, 64 functions. And variables. And macros. And a whole lotta jank.
This is not a professional application, and should not be treated as such. This is my ad-hoc calculator, and nothing more.

<img src="/Assets/graph.png" height="75%" width="75%">

To compile:

- Install Vulkan headers

- Install the Boost C++ library https://www.boost.org/

- Get a copy of NotoSansMathRegular

- Get a copy of ImGui and ImPlot and put their folders (named "imgui" and "implot") into the parent directory of where you copied this repo

- Check https://github.com/ocornut/imgui/blob/master/docs/FONTS.md#loading-font-data-embedded-in-source-code 

- Use https://github.com/ocornut/imgui/blob/master/misc/fonts/binary_to_compressed_c.cpp to turn the font into a C file.

- Rename that file to NotoSansMathRegular.hpp and make sure the names of the variables therein are "NotoSansMathRegular_compressed_size" and "NotoSansMathRegular_compressed_data"

- Copy implot.cpp, implot_demo.cpp, implot_items.cpp and imgui_stdlib.cpp into the folder with main.cpp

- Invoke cmake to build

- Pray it works lol

This readme currently sucks. In case you see this line, feel free to tell me to make it better, because I forgot or haven't gotten to it yet.

No coding agents or LLMs or similar were used for this. In case this line disappears, I have succumbed to the dark side.
