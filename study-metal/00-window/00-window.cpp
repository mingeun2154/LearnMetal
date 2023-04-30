/*
 *
 * Copyright 2022 Apple Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <iostream>
#include <cassert>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include "Metal/Metal.hpp"
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#pragma region Declarations {

/*
 * The Renderer class purpose is to draw whatever is contained
 * in the MTK::View object.
 * */
class Renderer
{
	public:
		Renderer(MTL::Device* pDevice);
		~Renderer();
		void draw(MTK::View* pView);

	private:
		MTL::Device* _pDevice;
		MTL::CommandQueue* _pCommandQueue;

};

class MyMTKViewDelegate : public MTK::ViewDelegate {
	public:
		MyMTKViewDelegate(MTL::Device* pDevice);
		virtual ~MyMTKViewDelegate() override;
		/* Called every frame(precisely, application loop) from the main evetn application loop.
		 * MTK::View forwards events to the view delegate class, such as a "render" event.
		 * drawInMTKView 함수가 호출되면 MTK::View 객체를 Renderer에게 전달하여 draw 함수를 호출한다.
		 * pRenderer->draw(pView)
		 * */
		virtual void drawInMTKView(MTK::View* pView) override;

	private:
		Renderer* _pRenderer;
};

/*
 * 애플리케이션이 macOS system으로부터 특정 Notification을 전달받았을때의 행동을 정의(customize)
 * */
class MyAppDelegate : public NS::ApplicationDelegate
{
	public:
		~MyAppDelegate();
		// 사용자가 윈도우를 조작할수 있는 다양한 메뉴를 만든다.
		NS::Menu* createMenuBar();
		/* Base class에 정의된 virtual function. 앱의 초기화가 거의 완료되었을때 호출된다.
		 * 주로 menu bar를 설정하거나 main event loop가 시작되기 전에 필요한 object들을 준비하는데 사용한다.
		 * */
		virtual void applicationWillFinishLaunching(NS::Notification* pNotification) override;
		/*
		 * 애플리케이션의 main event loop가 시작된 직후(아직 어떠한 event도 처리되지 않음)에 호출된다.
		 * window와 metal-related object들을 준비(setup)하기 위해 사용된다.
		 * */
		virtual void applicationDidFinishLaunching(NS::Notification* pNotification) override;
		// 모든 윈도우가 닫혔을때 애플리케이션이 main loop를 벗어나도록 한다.
		virtual bool applicationShouldTerminateAfterLastWindowClosed(NS::Application* pSender) override;

	private:
		NS::Window* _pWindow;
		MTK::View* _pMtkView;
		MTL::Device* _pDevice;
		MyMTKViewDelegate* _pViewDelegate = nullptr;
};

#pragma endregion Declarations }

int main() {
	std::cout << "Hello Metal\n";

	NS::AutoreleasePool* pAutoreleasePool = NS::AutoreleasePool::alloc()->init();

	MyAppDelegate del;

	NS::Application* pSharedApplication = NS::Application::sharedApplication();
	pSharedApplication->setDelegate(&del);
	pSharedApplication->run();

	pAutoreleasePool->release();

	return 0;
}

# pragma mark - AppDelegate
# pragma region AppDelegate

MyAppDelegate::~MyAppDelegate()
{
	_pWindow->release();
	_pMtkView->release();
	_pDevice->release();
	_pViewDelegate = nullptr;
}

/*
 * Menu* pMainMenu
 * ├── MenuItem* pAppMenuItem (Menu* pAppMenu)
 * │   └── Menu* pAppMenu
 * │       └── MenuItem* pAppQuitItem (callback)
 * └── MenuItem* pWindowMenuItem
 *     └── Menu* pWindowMenu
 *     	   └── MenuItem* pCloseWindowItem (callback)
 * */
NS::Menu* MyAppDelegate::createMenuBar() 
{
	using NS::StringEncoding::UTF8StringEncoding;

	NS::Menu* pMainMenu = NS::Menu::alloc()->init();

	// Menus for Application
	NS::MenuItem* pAppMenuItem = NS::MenuItem::alloc()->init();
	NS::Menu* pAppMenu = NS::Menu::alloc()->init(
			NS::String::string("Practice", UTF8StringEncoding));
	NS::String* appName = NS::RunningApplication::currentApplication()->localizedName();
	// 앱 종료 메뉴(기능) 등록
	NS::String* quitItemName = NS::String::string("Exit ", UTF8StringEncoding)->stringByAppendingString(appName);
	SEL quitCb = NS::MenuItem::registerActionCallback("exitApp", [](void*,SEL,const NS::Object* pSender){
			auto pApp = NS::Application::sharedApplication();
			pApp->terminate(pSender);
		});

	NS::MenuItem* pAppQuitItem = pAppMenu->addItem(quitItemName, quitCb, NS::String::string("q", UTF8StringEncoding));
	pAppQuitItem->setKeyEquivalentModifierMask(NS::EventModifierFlagCommand);
	pAppMenuItem->setSubmenu(pAppMenu);

	// Menus for Window
	NS::MenuItem* pWindowMenuItem = NS::MenuItem::alloc()->init();
	NS::Menu* pWindowMenu = NS::Menu::alloc()->init(
			NS::String::string("Window", UTF8StringEncoding));
	SEL closeWindowCb = NS::MenuItem::registerActionCallback("windowClose", [](void*, SEL, const NS::Object*){
			auto pApp = NS::Application::sharedApplication();
			pApp->windows()->object<NS::Window>(0)->close();
		});
	NS::MenuItem* pCloseWindowItem = pWindowMenu->addItem(
			NS::String::string("Close window", UTF8StringEncoding),
			closeWindowCb, NS::String::string("w", UTF8StringEncoding));
	pCloseWindowItem->setKeyEquivalentModifierMask(NS::EventModifierFlagCommand);
	pWindowMenuItem->setSubmenu(pWindowMenu);

	pMainMenu->addItem(pAppMenuItem);
	pMainMenu->addItem(pWindowMenuItem);

	pAppMenuItem->release();
	pWindowMenuItem->release();
	pAppMenu->release();
	pWindowMenu->release();

	return pMainMenu->autorelease();
}

void MyAppDelegate::applicationWillFinishLaunching(NS::Notification* pNotification) 
{
	NS::Menu* pMenu = createMenuBar();
	NS::Application* pApp = reinterpret_cast<NS::Application*>(pNotification->object());
	pApp->setMainMenu(pMenu);
	pApp->setActivationPolicy(NS::ActivationPolicyRegular);
}

void MyAppDelegate::applicationDidFinishLaunching(NS::Notification* pNotification)
{
	CGRect frame = (CGRect){{100.0, 100.0}, {512.0, 512.0}};

	// Window 객체 준비
	_pWindow = NS::Window::alloc()->init(
			frame, 
			NS::WindowStyleMaskClosable|NS::WindowStyleMaskTitled|NS::WindowStyleMaskResizable,
			NS::BackingStoreBuffered,
			false);
	// View 객체 준비
	_pDevice = MTL::CreateSystemDefaultDevice();
	_pMtkView = MTK::View::alloc()->init(frame, _pDevice);
	_pMtkView->setColorPixelFormat(MTL::PixelFormatBGRA8Unorm);
	_pMtkView->setClearColor(MTL::ClearColor::Make(1.0, 0.0, 0.0, 1.0));
	_pViewDelegate = new MyMTKViewDelegate(_pDevice);
	_pMtkView->setDelegate(_pViewDelegate);
	_pWindow->setContentView(_pMtkView);
	_pWindow->setTitle(NS::String::string("00 - Window 실습", NS::UTF8StringEncoding));
	_pWindow->makeKeyAndOrderFront(nullptr);
	// Application 객체 준비
	NS::Application* pApp = reinterpret_cast<NS::Application*>(pNotification->object());
	pApp->activateIgnoringOtherApps(true);
}

bool MyAppDelegate::applicationShouldTerminateAfterLastWindowClosed(
		NS::Application* pSender)
{
	return true;
}

# pragma endregion AppDelegate


# pragma mark - ViewDelegate
# pragma region ViewDelegate {

MyMTKViewDelegate::MyMTKViewDelegate(MTL::Device* pDevice)
: MTK::ViewDelegate(), _pRenderer(new Renderer(pDevice))
{
}

MyMTKViewDelegate::~MyMTKViewDelegate() 
{
	delete _pRenderer;
}

void MyMTKViewDelegate::drawInMTKView(MTK::View* pView)
{
	_pRenderer->draw(pView);
}

# pragma endregion ViewDelegate }


#pragma mark - Renderer
#pragma region Renderer {

Renderer::Renderer(MTL::Device* pDevice)
: _pDevice(pDevice->retain())
{
	_pCommandQueue = _pDevice->newCommandQueue();
}

Renderer::~Renderer()
{
	_pCommandQueue->release();
	_pDevice->release();
}

/*
 * Whent it is time to draw the screen, this function is called.
 * */
void Renderer::draw(MTK::View* pView)
{
	// AutoreleasePool for managing Metal objects.
	NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

	MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
	MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
	MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder(pRpd);
	pEnc->endEncoding();
	pCmd->presentDrawable(pView->currentDrawable());
	pCmd->commit();

	pPool->release();
}
#pragma endregion Renderer }
