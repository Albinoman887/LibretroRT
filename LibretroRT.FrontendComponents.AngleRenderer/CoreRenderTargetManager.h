#pragma once

using namespace LibretroRT;

using namespace Concurrency;

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
		unsigned int mTextureWidth, mTextureHeight;
		float mAspectRatio;
		PixelFormats mPixelFormat;

		GLuint mTextureID;
		critical_section mCriticalSection;

		void DeleteTexture();
		static void ConvertPixelFormat(PixelFormats libretroFormat);
	};
}
