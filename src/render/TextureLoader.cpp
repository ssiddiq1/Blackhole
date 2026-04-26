#include "render/TextureLoader.hpp"
#include <OpenGL/gl3.h>
#include <stb_image.h>
#include <iostream>
#include <vector>
#include <string>

unsigned int load_texture_2d(const std::string& path)
{
    GLuint tex;
    glGenTextures(1, &tex);

    int w, h, comp;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &comp, 0);
    if (data) {
        GLenum fmt, ifmt;
        if (comp == 1) {
            fmt = GL_RED;  ifmt = GL_RED;
        } else if (comp == 3) {
            fmt = GL_RGB;  ifmt = GL_SRGB;
        } else {
            fmt = GL_RGBA; ifmt = GL_SRGB_ALPHA;
        }
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    } else {
        std::cerr << "load_texture_2d: failed to load " << path << "\n";
    }
    return tex;
}

unsigned int load_cubemap(const std::string& dir)
{
    static const std::vector<std::string> faces =
        { "right", "left", "top", "bottom", "front", "back" };

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

    int w, h, comp;
    for (GLuint i = 0; i < (GLuint)faces.size(); ++i) {
        std::string p = dir + "/" + faces[i] + ".png";
        unsigned char* data = stbi_load(p.c_str(), &w, &h, &comp, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_SRGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cerr << "load_cubemap: failed to load " << p << "\n";
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return tex;
}
