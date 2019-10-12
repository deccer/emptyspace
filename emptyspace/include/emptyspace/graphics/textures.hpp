#pragma once

#include <emptyspace/types.hpp>

#include <stb_image.h>

#include <glad/glad.h>
#include <string_view>

class Texture
{
public:
	static Texture* FromFile(const std::string_view filepath, u32 comp = STBI_rgb_alpha);

	Texture(const u32 internalFormat, const u32 format, const s32 width, const s32 height, void* data = nullptr, const u32 filter = GL_LINEAR, const u32 repeat = GL_REPEAT);
	~Texture();

	[[nodiscard]] u32 Id() const;
private:
	u32 _id{};
};