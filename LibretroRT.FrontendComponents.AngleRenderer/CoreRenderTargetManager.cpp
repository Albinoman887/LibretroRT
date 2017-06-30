#include "pch.h"
#include "CoreRenderTargetManager.h"
#include "OpenGLES.h"

using namespace LibretroRT_FrontendComponents_AngleRenderer;

using namespace Concurrency;
using namespace Platform;

const unsigned int CoreRenderTargetManager::MinTextureSize = 512;

CoreRenderTargetManager::CoreRenderTargetManager() :
	mTextureSize(MinTextureSize),
	mAspectRatio(0.0f),
	mPixelFormat(PixelFormats::FormatUknown),
	mPixelFormatBPP(0),
	mTextureID(GL_NONE),
	mVertexPositionBufferID(GL_NONE),
	mVertexTexPositionBufferID(GL_NONE),
	mIndexBufferID(GL_NONE),
	mProgramID(GL_NONE)
{
	static const std::string vertexShader(R"SHADER(
attribute vec4 a_position;
attribute vec2 a_texture_position;
varying vec2 v_texture_position;
uniform mat4 u_matrix;
uniform mat4 u_texture_matrix;
void main()
{
  v_texture_position = (u_texture_matrix * vec4(a_texture_position, 0.0, 1.0)).xy;
  gl_Position = u_matrix * a_position;
}
)SHADER");

	static const std::string pixelShader(R"SHADER(
precision mediump float;
varying vec2 v_texture_position;
uniform sampler2D u_texture_unit;
void main()
{
  gl_FragColor = texture2D(u_texture_unit, v_texture_position);
}
)SHADER");

	mProgramID = OpenGLES::CompileProgram(vertexShader, pixelShader);

	GLfloat positions[] =
	{
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f
	};

	glGenBuffers(1, &mVertexPositionBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexPositionBufferID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

	glGenBuffers(1, &mVertexTexPositionBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexTexPositionBufferID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

	short indices[] =
	{
		0, 1, 2,
		2, 3, 0
	};

	glGenBuffers(1, &mIndexBufferID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBufferID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

CoreRenderTargetManager::~CoreRenderTargetManager()
{
	critical_section::scoped_lock lock(mCriticalSection);
	DeleteTexture();

	if (mProgramID != GL_NONE)
	{
		glDeleteProgram(mProgramID);
		mProgramID = GL_NONE;
	}

	if (mVertexPositionBufferID != GL_NONE)
	{
		glDeleteBuffers(1, &mVertexPositionBufferID);
		mVertexPositionBufferID = GL_NONE;
	}

	if (mVertexTexPositionBufferID != GL_NONE)
	{
		glDeleteBuffers(1, &mVertexTexPositionBufferID);
		mVertexTexPositionBufferID = GL_NONE;
	}

	if (mIndexBufferID != GL_NONE)
	{
		glDeleteBuffers(1, &mIndexBufferID);
		mIndexBufferID = GL_NONE;
	}
}

void CoreRenderTargetManager::SetFormat(GameGeometry^ geometry, PixelFormats pixelFormat)
{
	auto requestedSize = max(MinTextureSize, max(geometry->MaxWidth, geometry->MaxHeight));
	requestedSize = GetClosestPowerOfTwo(requestedSize);
	if (requestedSize < mTextureSize && pixelFormat == mPixelFormat)
	{
		return;
	}

	critical_section::scoped_lock lock(mCriticalSection);
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
	critical_section::scoped_lock lock(mCriticalSection);
	if (mTextureID == GL_INVALID_VALUE || frameBuffer == nullptr || mPixelFormatBPP == 0)
	{
		return;
	}

	width = pitch / mPixelFormatBPP;
	glBindTexture(GL_TEXTURE_2D, mTextureID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer->Data);
}

void CoreRenderTargetManager::Render(EGLint canvasWidth, EGLint canvasHeight)
{
	critical_section::scoped_lock lock(mCriticalSection);
	if (mTextureID == GL_INVALID_VALUE || mAspectRatio < 0.1)
	{
		return;
	}
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
