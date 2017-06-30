#include "pch.h"
#include "CoreRenderTargetManager.h"
#include "OpenGLES.h"

using namespace LibretroRT_FrontendComponents_AngleRenderer;

using namespace Platform;

CoreRenderTargetManager::CoreRenderTargetManager() :
	mTextureSize(512),
	mAspectRatio(0.0f),
	mPixelFormat(PixelFormats::FormatUknown),
	mPixelFormatBPP(0),
	mTextureID(GL_NONE)
{
}


CoreRenderTargetManager::~CoreRenderTargetManager()
{
	DeleteTexture();
}

void CoreRenderTargetManager::SetFormat(GameGeometry^ geometry, PixelFormats pixelFormat)
{
	auto requestedSize = max(geometry->MaxWidth, geometry->MaxHeight);
	requestedSize = GetClosestPowerOfTwo(requestedSize);
	if (requestedSize < mTextureSize && pixelFormat == mPixelFormat)
	{
		return;
	}

	DeleteTexture();

	mTextureSize = requestedSize;
	auto glPixelFormat = ConvertPixelFormat(pixelFormat);
	glGenTextures(1, &mTextureID);
	glBindTexture(GL_TEXTURE_2D, mTextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, glPixelFormat, mTextureSize, mTextureSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
}

void CoreRenderTargetManager::UpdateFromCoreOutput(const Array<byte>^ frameBuffer, unsigned int width, unsigned int height, unsigned int pitch)
{
	if (mTextureID == GL_INVALID_VALUE)
	{
		return;
	}

	width = pitch / mPixelFormatBPP;
	glBindTexture(GL_TEXTURE_2D, mTextureID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer->Data);
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
		return GL_RGB5_A1;
	case LibretroRT::PixelFormats::FormatXRGB8888:
		return GL_RGBA;
	case LibretroRT::PixelFormats::FormatRGB565:
		return GL_RGB565;
	}

	return GL_INVALID_VALUE;
}

unsigned int CoreRenderTargetManager::ConvertPixelFormatToBPP(PixelFormats libretroFormat)
{
	switch (libretroFormat)
	{
	case LibretroRT::PixelFormats::Format0RGB1555:
		return 2;
	case LibretroRT::PixelFormats::FormatXRGB8888:
		return 4;
	case LibretroRT::PixelFormats::FormatRGB565:
		return 2;
	}

	return 0;
}

unsigned int CoreRenderTargetManager::GetClosestPowerOfTwo(unsigned int value)
{
	unsigned int output = 1;
	while (output < value)
	{
		output *= 2;
	}

	return output;
}