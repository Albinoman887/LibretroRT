#include "pch.h"
#include "CoreRenderTargetManager.h"
#include "OpenGLES.h"

using namespace LibretroRT_FrontendComponents_AngleRenderer;

CoreRenderTargetManager::CoreRenderTargetManager() :
	mTextureWidth(0),
	mTextureHeight(0),
	mAspectRatio(0.0f),
	mPixelFormat(PixelFormats::FormatUknown),
	mTextureID(GL_NONE)
{
}


CoreRenderTargetManager::~CoreRenderTargetManager()
{
}

void CoreRenderTargetManager::SetFormat(GameGeometry^ geometry, PixelFormats pixelFormat)
{
	DeleteTexture();

	glGenTextures(1, &mTextureID);
	glBindTexture(GL_TEXTURE_2D, mTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, geometry->MaxWidth, geometry->MaxHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
}

void CoreRenderTargetManager::DeleteTexture()
{
	if (mTextureID == GL_NONE)
	{
		return;
	}

	glBindTexture(GL_TEXTURE_2D, mTextureID);
	glDeleteTextures(1, &mTextureID);
	mTextureID = GL_NONE;
}

GLint CoreRenderTargetManager::ConvertPixelFormat(PixelFormats libretroFormat)
{
	switch (libretroFormat)
	{
	case LibretroRT::PixelFormats::Format0RGB1555:
		return GL_INVALID_VALUE;
	case LibretroRT::PixelFormats::FormatXRGB8888:
		return GL_RGBA;
	case LibretroRT::PixelFormats::FormatRGB565:
		return GL_RGB565;
	case LibretroRT::PixelFormats::FormatUknown:
		return GL_INVALID_VALUE;
	default:
		return 0;
	}
}