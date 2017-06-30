#pragma once

using namespace Windows::Foundation;
using namespace Windows::UI::Xaml::Controls;

namespace LibretroRT_FrontendComponents_AngleRenderer
{
	class OpenGLES
	{
	public:
		~OpenGLES();

		static std::shared_ptr<OpenGLES>& GetInstance();
		EGLSurface CreateSurface(SwapChainPanel^ panel, const Size* renderSurfaceSize, const float* renderResolutionScale);
		void GetSurfaceDimensions(const EGLSurface surface, EGLint *width, EGLint *height);
		void DestroySurface(const EGLSurface surface);
		static GLuint CompileProgram(const std::string &vsSource, const std::string &fsSource);

		void MakeCurrent(const EGLSurface surface);
		EGLBoolean SwapBuffers(const EGLSurface surface);
		void Reset();

	private:
		static std::mutex mMutex;

		OpenGLES();
		OpenGLES(const OpenGLES& rs);
		OpenGLES& operator = (const OpenGLES& rs);

		void Initialize();
		void Cleanup();
		static GLuint CompileShader(GLenum type, const std::string &source);

		EGLDisplay mEglDisplay;
		EGLContext mEglContext;
		EGLConfig  mEglConfig;
	};
}
