#include "pch.h"
#include "AngleRenderer.h"

using namespace Platform;
using namespace Concurrency;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;

using namespace LibretroRT_FrontendComponents_AngleRenderer;

AngleRenderer::AngleRenderer(SwapChainPanel^ swapChainPanel, IAudioPlayer^ audioPlayer, IInputManager^ inputManager) :
	mGameId(nullptr),
	mCoordinator(ref new CoreCoordinator),
	mOpenGLES(*OpenGLES::GetInstance()),
	mSwapChainPanel(swapChainPanel),
	mRenderSurface(EGL_NO_SURFACE)
{
	mCoordinator->Renderer = this;
	mCoordinator->AudioPlayer = audioPlayer;
	mCoordinator->InputManager = inputManager;

	//CoreWindow^ window = Window::Current->CoreWindow;
	//auto token = window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &AngleRenderer::OnVisibilityChanged);
	mOnPageLoadedRegistrationToken = mSwapChainPanel->Loaded += ref new RoutedEventHandler(this, &AngleRenderer::OnPageLoaded);
}

AngleRenderer::~AngleRenderer()
{
	mSwapChainPanel->Loaded -= mOnPageLoadedRegistrationToken;

	StopRendering();

	auto core = mCoordinator->Core;
	if (core) { core->UnloadGame(); }

	DestroyRenderSurface();
}

IAsyncOperation<bool>^ AngleRenderer::LoadGameAsync(ICore^ core, String^ mainGameFilePath)
{
	return create_async([=]
	{
		create_task(UnloadGameAsync()).wait();

		mCoordinator->Core = core;
		if (!core->LoadGame(mainGameFilePath))
		{
			return false;
		}

		GameID = mainGameFilePath;
		StartRendering();
		return true;
	});
}

IAsyncAction^ AngleRenderer::UnloadGameAsync()
{
	return create_async([=]
	{
		StopRendering();
		GameID = nullptr;

		auto core = mCoordinator->Core;
		if (core) { core->UnloadGame(); }

		auto audioPlayer = mCoordinator->AudioPlayer;
		if (audioPlayer) { audioPlayer->Stop(); }
	});
}

IAsyncAction^ AngleRenderer::ResetGameAsync()
{
	return create_async([=]
	{
		critical_section::scoped_lock lock(mCoordinatorCriticalSection);

		auto audioPlayer = mCoordinator->AudioPlayer;
		if (audioPlayer) { audioPlayer->Stop(); }

		auto core = mCoordinator->Core;
		if (core) { core->Reset(); }
	});
}

IAsyncAction^ AngleRenderer::PauseCoreExecutionAsync()
{
	return create_async([=]
	{
		StopRendering();

		auto audioPlayer = mCoordinator->AudioPlayer;
		if (audioPlayer) { audioPlayer->Stop(); }
	});
}

IAsyncAction^ AngleRenderer::ResumeCoreExecutionAsync()
{
	return create_async([=]
	{
		StartRendering();
	});
}

IAsyncOperation<bool>^ AngleRenderer::SaveGameStateAsync(WriteOnlyArray<byte>^ stateData)
{
	return create_async([=]
	{
		critical_section::scoped_lock lock(mCoordinatorCriticalSection);

		auto core = mCoordinator->Core;
		if (!core) { return false; }

		return core->Serialize(stateData);
	});
}

IAsyncOperation<bool>^ AngleRenderer::LoadGameStateAsync(const Array<byte>^ stateData)
{
	return create_async([=]
	{
		critical_section::scoped_lock lock(mCoordinatorCriticalSection);

		auto core = mCoordinator->Core;
		if (!core) { return false; }

		return core->Unserialize(stateData);
	});
}

void AngleRenderer::GeometryChanged(GameGeometry^ geometry)
{
	auto core = mCoordinator->Core;
	if (!core) { return; }

	if (mRenderTargetManager)
	{
		mRenderTargetManager->SetFormat(geometry, core->PixelFormat);
	}
}

void AngleRenderer::PixelFormatChanged(PixelFormats format)
{
	auto core = mCoordinator->Core;
	if (!core) { return; }

	if (mRenderTargetManager)
	{
		mRenderTargetManager->SetFormat(core->Geometry, format);
	}
}

void AngleRenderer::RenderVideoFrame(const Array<byte>^ frameBuffer, unsigned int width, unsigned int height, unsigned int pitch)
{
	if (mRenderTargetManager)
	{
		mRenderTargetManager->UpdateFromCoreOutput(frameBuffer, width, height, pitch);
	}
}

void AngleRenderer::OnPageLoaded(Platform::Object^ sender, RoutedEventArgs^ e)
{
	// The SwapChainPanel has been created and arranged in the page layout, so EGL can be initialized.
	CreateRenderSurface();
}

/*void AngleRenderer::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	if (args->Visible && mRenderSurface != EGL_NO_SURFACE)
	{
		StartRendering();
	}
	else
	{
		StopRendering();
	}
}*/

void AngleRenderer::CreateRenderSurface()
{
	if (mRenderSurface != EGL_NO_SURFACE)
	{
		return;
	}

	// The app can configure the the SwapChainPanel which may boost performance.
	// By default, this template uses the default configuration.
	mRenderSurface = mOpenGLES.CreateSurface(mSwapChainPanel, nullptr, nullptr);

	// You can configure the SwapChainPanel to render at a lower resolution and be scaled up to
	// the swapchain panel size. This scaling is often free on mobile hardware.
	//
	// One way to configure the SwapChainPanel is to specify precisely which resolution it should render at.
	// Size customRenderSurfaceSize = Size(800, 600);
	// mRenderSurface = mOpenGLES->CreateSurface(swapChainPanel, &customRenderSurfaceSize, nullptr);
	//
	// Another way is to tell the SwapChainPanel to render at a certain scale factor compared to its size.
	// e.g. if the SwapChainPanel is 1920x1280 then setting a factor of 0.5f will make the app render at 960x640
	// float customResolutionScale = 0.5f;
	// mRenderSurface = mOpenGLES->CreateSurface(swapChainPanel, nullptr, &customResolutionScale);
	//
}

void AngleRenderer::DestroyRenderSurface()
{
	mOpenGLES.DestroySurface(mRenderSurface);
	mRenderSurface = EGL_NO_SURFACE;
}

void AngleRenderer::RecoverFromLostDevice()
{
	// Stop the render loop, reset OpenGLES, recreate the render surface
	// and start the render loop again to recover from a lost device.

	StopRendering();

	DestroyRenderSurface();
	mOpenGLES.Reset();
	CreateRenderSurface();

	StartRendering();
}

void AngleRenderer::StartRendering()
{
	// If the render loop is already running then do not start another thread.
	if (mRenderLoopWorker != nullptr && mRenderLoopWorker->Status == AsyncStatus::Started)
	{
		return;
	}

	// Create a task for rendering that will be run on a background thread.
	auto workItemHandler = ref new Windows::System::Threading::WorkItemHandler([this](IAsyncAction ^ action)
	{
		critical_section::scoped_lock lock(mRenderSurfaceCriticalSection);

		mOpenGLES.MakeCurrent(mRenderSurface);

		mRenderTargetManager = std::make_unique<CoreRenderTargetManager>();
		auto core = mCoordinator->Core;
		mRenderTargetManager->SetFormat(core->Geometry, core->PixelFormat);

		while (action->Status == AsyncStatus::Started)
		{
			EGLint panelWidth = 0;
			EGLint panelHeight = 0;
			mOpenGLES.GetSurfaceDimensions(mRenderSurface, &panelWidth, &panelHeight);

			RunFrameCoreLogic();
			mRenderTargetManager->Render(panelWidth, panelHeight);
			
			// The call to eglSwapBuffers might not be successful (i.e. due to Device Lost)
			// If the call fails, then we must reinitialize EGL and the GL resources.
			if (mOpenGLES.SwapBuffers(mRenderSurface) != GL_TRUE)
			{
				// XAML objects like the SwapChainPanel must only be manipulated on the UI thread.
				mSwapChainPanel->Dispatcher->RunAsync(CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
				{
					RecoverFromLostDevice();
				}, CallbackContext::Any));

				return;
			}
		}

		mRenderTargetManager.reset();
	});

	// Run task on a dedicated high priority background thread.
	mRenderLoopWorker = Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, Windows::System::Threading::WorkItemPriority::High, Windows::System::Threading::WorkItemOptions::TimeSliced);
}

void AngleRenderer::StopRendering()
{
	if (mRenderLoopWorker)
	{
		mRenderLoopWorker->Cancel();
		mRenderLoopWorker = nullptr;

		critical_section::scoped_lock lock(mRenderSurfaceCriticalSection);
	}
}

void AngleRenderer::RunFrameCoreLogic()
{
	critical_section::scoped_lock lock(mCoordinatorCriticalSection);

	auto core = mCoordinator->Core;
	try
	{
		core->RunFrame();
	}
	catch(Platform::FailureException^ e)
	{
		HResult res;
		res.Value = e->HResult;
		CoreRunExceptionOccurred(core, res);
	}
}