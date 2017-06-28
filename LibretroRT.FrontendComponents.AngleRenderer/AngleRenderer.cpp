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
	mCoreIsExecuting(false),
	mCoordinator(ref new CoreCoordinator),
	mOpenGLES(*OpenGLES::GetInstance()),
	mSwapChainPanel(swapChainPanel),
	mRenderSurface(EGL_NO_SURFACE)
{
	mCoordinator->Renderer = this;
	mCoordinator->AudioPlayer = audioPlayer;
	mCoordinator->InputManager = inputManager;

	CoreWindow^ window = Window::Current->CoreWindow;
	auto token = window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &AngleRenderer::OnVisibilityChanged);
	mSwapChainPanel->Loaded += ref new RoutedEventHandler(this, &AngleRenderer::OnPageLoaded);
}

AngleRenderer::~AngleRenderer()
{
	critical_section::scoped_lock lock(mCoordinatorCriticalSection);
	auto core = mCoordinator->Core;
	if (core) { core->UnloadGame(); }

	DestroyRenderSurface();
}

IAsyncOperation<bool>^ AngleRenderer::LoadGameAsync(ICore^ core, String^ mainGameFilePath)
{
	return create_async([=]
	{
		/*while (!RenderPanelInitialized)
		{
		//Ensure core doesn't try rendering before Win2D is ready.
		//Some games load faster than the Win2D canvas is initialized
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}*/

		auto output = create_task(UnloadGameAsync()).then([=]
		{
			critical_section::scoped_lock lock(mCoordinatorCriticalSection);

			mCoordinator->Core = core;
			if (!core->LoadGame(mainGameFilePath))
			{
				return false;
			}

			GameID = mainGameFilePath;
			//RenderTargetManager.CurrentCorePixelFormat = core.PixelFormat;
			CoreIsExecuting = true;
			return true;
		});

		return output;
	});
}

IAsyncAction^ AngleRenderer::UnloadGameAsync()
{
	return create_async([=]
	{
		critical_section::scoped_lock lock(mCoordinatorCriticalSection);

		GameID = nullptr;
		CoreIsExecuting = false;

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
		critical_section::scoped_lock lock(mCoordinatorCriticalSection);

		auto audioPlayer = mCoordinator->AudioPlayer;
		if (audioPlayer) { audioPlayer->Stop(); }

		CoreIsExecuting = false;
	});
}

IAsyncAction^ AngleRenderer::ResumeCoreExecutionAsync()
{
	return create_async([=]
	{
		critical_section::scoped_lock lock(mCoordinatorCriticalSection);

		CoreIsExecuting = true;
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

void AngleRenderer::OnPageLoaded(Platform::Object^ sender, RoutedEventArgs^ e)
{
	// The SwapChainPanel has been created and arranged in the page layout, so EGL can be initialized.
	CreateRenderSurface();
}

void AngleRenderer::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	if (args->Visible && mRenderSurface != EGL_NO_SURFACE)
	{
		StartRenderer();
	}
	else
	{
		StopRenderer();
	}
}

void AngleRenderer::CreateRenderSurface()
{
	if (mRenderSurface == EGL_NO_SURFACE)
	{
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

	StopRenderer();

	{
		critical_section::scoped_lock lock(mRenderSurfaceCriticalSection);

		DestroyRenderSurface();
		mOpenGLES.Reset();
		CreateRenderSurface();
	}

	StartRenderer();
}

void AngleRenderer::StartRenderer(IRenderer^ renderer)
{
	StopRenderer();
	
	{
		critical_section::scoped_lock lock(mRenderSurfaceCriticalSection);

		mRenderer = renderer;
	}

	StartRenderer();
}

void AngleRenderer::StartRenderer()
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

		mRenderer->Init();

		while (action->Status == AsyncStatus::Started)
		{
			EGLint panelWidth = 0;
			EGLint panelHeight = 0;
			mOpenGLES.GetSurfaceDimensions(mRenderSurface, &panelWidth, &panelHeight);

			// Logic to update the scene could go here
			mRenderer->UpdateWindowSize(panelWidth, panelHeight);
			mRenderer->Draw();

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

		mRenderer->Deinit();
	});

	// Run task on a dedicated high priority background thread.
	mRenderLoopWorker = Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, Windows::System::Threading::WorkItemPriority::High, Windows::System::Threading::WorkItemOptions::TimeSliced);
}

void AngleRenderer::StopRenderer()
{
	if (mRenderLoopWorker)
	{
		mRenderLoopWorker->Cancel();
		mRenderLoopWorker = nullptr;
	}
}