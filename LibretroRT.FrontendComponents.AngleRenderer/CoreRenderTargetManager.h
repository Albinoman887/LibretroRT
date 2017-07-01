#pragma once

#include "Matrix4.h"

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
		std::vector<unsigned char> mTextureBuffer;
		GLuint mVertexPositionBufferID;
		GLuint mVertexTexturePositionBufferID;
		GLuint mIndexBufferID;
		GLuint mProgramID;

		Matrix4 mTextureMatrix;

		GLint mPositionAttribLocation;
		GLint mTexturePositionAttribLocation;
		GLint mMatrixUniformLocation;
		GLint mTextureMatrixUniformLocation;
		GLint mTextureUnitUniformlocation;

		critical_section mCriticalSection;

		void DeleteTexture();
		static bool GenerateTextureParametersForPixelFormat(PixelFormats libretroFormat, GLenum* textureFormat, GLenum* textureDataType, GLuint* BPP);
		static unsigned int GetClosestPowerOfTwo(unsigned int value);
		static Matrix4 ComputeFittingMatrix(float viewportAspectRatio, float aspectRatio);
	};
}
