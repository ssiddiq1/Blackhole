#pragma once
#include <string>

// load_texture_2d: loads PNG/JPG → GL_TEXTURE_2D, CLAMP_TO_EDGE, LINEAR mips.
unsigned int load_texture_2d(const std::string& path);

// load_cubemap: loads right/left/top/bottom/front/back.png from dir → GL_TEXTURE_CUBE_MAP.
unsigned int load_cubemap(const std::string& dir);
