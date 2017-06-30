#pragma once

using namespace LibretroRT;

using namespace Concurrency;
using namespace Platform;

namespace LibretroRT_FrontendComponents_AngleRenderer
{
	class CoreRenderTargetManager
	{
	public:
		CoreRenderTargetManager();
		~CoreRenderTargetManager();

		void SetFormat(GameGeometry^ geometry, PixelFormats pixelFormat);
		void UpdateFromCoreOutput(const Array<byte>^ frameBuffer, unsigned int width, unsigned int height, unsigned int pitch);
		void Render(EGLint canvasWidth, EGLint canvasHeight);

	private:
		static const unsigned int MinTextureSize;
		unsigned int mTextureSize;
		float mAspectRatio;
		PixelFormats mPixelFormat;

		GLuint mTextureID;
		GLuint mVertexPositionBufferID;
		GLuint mVertexTexturePositionBufferID;
		GLuint mIndexBufferID;
		GLuint mProgramID;

		GLint mPositionAttribLocation;
		GLint mTexturePositionAttribLocation;
		GLint mMatrixUniformLocation;
		GLint mTextureMatrixUniformLocation;

		critical_section mCriticalSection;

		void DeleteTexture();
		static GLint ConvertPixelFormat(PixelFormats libretroFormat);
		static unsigned int GetClosestPowerOfTwo(unsigned int value);
	};
}
