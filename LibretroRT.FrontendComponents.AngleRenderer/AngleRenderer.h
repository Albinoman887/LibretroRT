#pragma once

#include "OpenGLES.h"
#include <concrt.h>

using namespace LibretroRT;
using namespace LibretroRT::FrontendComponents::Common;

using namespace Concurrency;
using namespace Platform;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

namespace LibretroRT_FrontendComponents_AngleRenderer
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class AngleRenderer sealed : IRenderer, ICoreRunner
	{
	public:
		virtual property String^ GameID
		{
			String^ get() { return mGameId; }
		private:
			void set(String^ value) { mGameId = value; }
		}

		virtual property bool CoreIsExecuting
		{
			bool get() { return mRenderLoopWorker != nullptr; }
		}

		virtual property unsigned int SerializationSize
		{
			unsigned int get()
			{
				critical_section::scoped_lock lock(mCoordinatorCriticalSection);
				auto core = mCoordinator->Core;
				return core == nullptr ? 0 : core->SerializationSize;
			}
		}

		virtual IAsyncOperation<bool>^ LoadGameAsync(ICore^ core, String^ mainGameFilePath) override;
		virtual IAsyncAction^ UnloadGameAsync() override;
		virtual IAsyncAction^ ResetGameAsync() override;

		virtual IAsyncAction^ PauseCoreExecutionAsync() override;
		virtual IAsyncAction^ ResumeCoreExecutionAsync() override;

		virtual IAsyncOperation<bool>^ SaveGameStateAsync(WriteOnlyArray<byte>^ stateData) override;
		virtual IAsyncOperation<bool>^ LoadGameStateAsync(const Array<byte>^ stateData) override;

		virtual event CoreRunExceptionOccurredDelegate^ CoreRunExceptionOccurred;

		virtual void GeometryChanged(GameGeometry^ geometry) override;
		virtual void PixelFormatChanged(PixelFormats format) override;
		virtual void RenderVideoFrame(const Array<byte>^ frameBuffer, unsigned int width, unsigned int height, unsigned int pitch) override;

		AngleRenderer(SwapChainPanel^ swapChainPanel, IAudioPlayer^ audioPlayer, IInputManager^ inputManager);
		virtual ~AngleRenderer();

	private:
		String^ mGameId;

		CoreCoordinator^ const mCoordinator;
		critical_section mCoordinatorCriticalSection;

		OpenGLES& mOpenGLES;
		SwapChainPanel^ mSwapChainPanel;
		EGLSurface mRenderSurface;
		IAsyncAction^ mRenderLoopWorker;
		critical_section mRenderSurfaceCriticalSection;

		void CreateRenderSurface();
		void DestroyRenderSurface();
		void RecoverFromLostDevice();
		void StartRendering();
		void StopRendering();
		void RunFrameCoreLogic();
		void OnPageLoaded(Platform::Object^ sender, RoutedEventArgs^ e);
		//void OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args);
	};
}


