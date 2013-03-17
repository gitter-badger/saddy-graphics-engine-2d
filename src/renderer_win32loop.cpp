#include "renderer.h"
#include "input.h"



#ifdef WIN32


#include <windowsx.h>

#ifndef GET_KEYSTATE_WPARAM 
	#define GET_KEYSTATE_WPARAM(wParam)     (LOWORD(wParam))
#endif

#ifndef GET_KEYSTATE_LPARAM
	#define GET_KEYSTATE_LPARAM(lParam) (LOWORD(lParam))
#endif

void sad::Renderer::mainLoop()
{
 int frames=0;
 m_fps=60;
 //bool isMessagePumpActive;
 MSG msg;

 m_running = true;											// Program Looping Is Set To TRUE
 m_window.active=true;
 Renderer::setTimer();


 while (m_running)											// Loop Until WM_QUIT Is Received
 {					
  // Check For Window Messages
  if (PeekMessage (&msg, m_window.hWND, 0, 0, PM_REMOVE) != 0)
  {
	 // Check For WM_QUIT Message
	 if (msg.message != WM_QUIT)						// Is The Message A WM_QUIT Message?
	 {
	  TranslateMessage(&msg);				// Translate The Message
	  DispatchMessage(&msg);						// If Not, Dispatch The Message
	 }
	 else											// Otherwise (If Message Is WM_QUIT)
	 {
		m_running = false;				// Terminate The Message Loop
	 }
   }
   else												// If There Are No Messages
   {
	  // Process Application Loop
	  frames++;
	  if (Renderer::ref()->elapsedInMSeconds() >= 1000)
	  {
		  setFPS(frames);frames=0;Renderer::ref()->setTimer();
	  }
	  //Update a window, if active
	  if (m_window.active)
	     update();
	  //Change scene, if need so
	  if (m_chscene) 
	  { setCurrentScene(m_chscene); m_chscene=NULL;}
   }
  }
 sad::Input::inst()->postQuit();
 m_window.active=false;
 this->releaseWindow();
}


static hst::hash<WPARAM,int> table;

static void table_init()
{
   for (WPARAM t=VK_F1;t<=VK_F12;t++)
   table.insert(t,VK_F12-VK_F1+KEY_F1+t);
   table.insert(VK_ESCAPE,KEY_ESC);
   table.insert(VK_PAUSE,KEY_PAUSE);
   table.insert(VK_INSERT,KEY_INSERT);
   table.insert(VK_SNAPSHOT,KEY_PRINTSCREEN);
   table.insert(VK_DELETE,KEY_DELETE);
   table.insert(VK_BACK,KEY_BACKSPACE);
   table.insert(VK_TAB,KEY_TAB);
   table.insert(VK_NUMLOCK,KEY_NUMLOCK);
   table.insert(VK_SCROLL,KEY_SCROLLOCK);
   table.insert(VK_CAPITAL,KEY_CAPSLOCK);
   table.insert(VK_LSHIFT,KEY_LSHIFT);
   table.insert(VK_LCONTROL,KEY_LCTRL);
   table.insert(VK_LWIN,KEY_WIN);
   table.insert(VK_LMENU,KEY_LALT);
   table.insert(VK_APPS,KEY_APP);
   table.insert(VK_SPACE,KEY_SPACE);
   table.insert(VK_RMENU,KEY_RALT);
   table.insert(VK_RCONTROL,KEY_RCTRL);
   table.insert(VK_LEFT,KEY_LEFT);
   table.insert(VK_RIGHT,KEY_RIGHT);
   table.insert(VK_UP,KEY_UP);
   table.insert(VK_DOWN,KEY_DOWN);
   table.insert(VK_RETURN,KEY_ENTER);
   table.insert(VK_RSHIFT,KEY_RSHIFT);
   table.insert(VK_HOME,KEY_HOME);
   table.insert(VK_END,KEY_END);
   table.insert(VK_PRIOR,KEY_PGUP);
   table.insert(VK_NEXT,KEY_PGDOWN);
}


POINT sad::Renderer::_toClient(POINT  _pnt)
{
	POINT pnt;
	pnt.x = _pnt.x;
	pnt.y = _pnt.y;
	RECT r;
	GetClientRect(ref()->m_window.hWND, &r);
	pnt.y = r.bottom - pnt.y;
	return pnt;
}

POINT sad::Renderer::_toClient(LPARAM lParam)
{
	POINT pnt;
	pnt.x = GET_X_LPARAM(lParam);
	pnt.y = GET_Y_LPARAM(lParam);
	return sad::Renderer::_toClient(pnt);
}

LRESULT CALLBACK sad::Renderer::WindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool msg_init=false;
	if (!msg_init)
	{
		msg_init=true;
		table_init();
	}

	if (uMsg==WM_CLOSE)
	{
     	    sad::Renderer::ref()->m_running=false;					
			return 0;													
	}
	if (uMsg==WM_MOUSEMOVE)
	{
		if (sad::Input::inst()->areMovingNotTracked()) 
			return 0;
		float mx=0,my=0,mz=0;
		int key=(wParam==MK_LBUTTON)?MOUSE_BUTTON_LEFT:(wParam==MK_RBUTTON)?MOUSE_BUTTON_RIGHT:(wParam==MK_MBUTTON)?MOUSE_BUTTON_MIDDLE:0;
		POINT pnt = ref()->_toClient(lParam);
		ref()->mapToOGL(pnt.x,pnt.y,mx,my,mz);
		sad::Input::inst()->postMouseMove(sad::Event(mx,my,mz,key));
	}
	if (uMsg==WM_MOUSEWHEEL)
	{
		if (sad::Input::inst()->areWheelNotTracked()) 
			return 0;
		float mx=0,my=0,mz=0;
		float fw=GET_WHEEL_DELTA_WPARAM(wParam)/(float)WHEEL_DELTA;
		wParam=GET_KEYSTATE_WPARAM(wParam);
		int key=(wParam==MK_LBUTTON)?MOUSE_BUTTON_LEFT:(wParam==MK_RBUTTON)?MOUSE_BUTTON_RIGHT:(wParam==MK_MBUTTON)?MOUSE_BUTTON_MIDDLE:0;
		POINT pnt = ref()->_toClient(lParam);
		ref()->mapToOGL(pnt.x,pnt.y,mx,my,mz);
		sad::Event ev(mx,my,mz,key);
		ev.delta=fw;
		sad::Input::inst()->postMouseWheel(ev);
		return 0;
	}
	if (uMsg==WM_LBUTTONDBLCLK || uMsg==WM_MBUTTONDBLCLK || uMsg==WM_RBUTTONDBLCLK)
	{
		if (sad::Input::inst()->areDblClickNotTracked())
			return 0;
		float mx=0,my=0,mz=0;
		int key=(wParam==MK_LBUTTON)?MOUSE_BUTTON_LEFT:(wParam==MK_RBUTTON)?MOUSE_BUTTON_RIGHT:(wParam==MK_MBUTTON)?MOUSE_BUTTON_MIDDLE:0;
		POINT pnt = ref()->_toClient(lParam);
		ref()->mapToOGL(pnt.x,pnt.y,mx,my,mz);
		sad::Input::inst()->postMouseDblClick(sad::Event(mx,my,mz,key));
		return 0;
	}
	if (uMsg==WM_LBUTTONUP || uMsg==WM_MBUTTONUP || uMsg==WM_RBUTTONUP)
	{
		if (sad::Input::inst()->areUpNotTracked())
			return 0;
		float mx=0,my=0,mz=0;
		int key=(wParam==MK_LBUTTON)?MOUSE_BUTTON_LEFT:(wParam==MK_RBUTTON)?MOUSE_BUTTON_RIGHT:(wParam==MK_MBUTTON)?MOUSE_BUTTON_MIDDLE:0;
		POINT pnt = ref()->_toClient(lParam);
		ref()->mapToOGL(pnt.x,pnt.y,mx,my,mz);
		sad::Input::inst()->postMouseUp(sad::Event(mx,my,mz,key));
		return 0;
	}
	if (uMsg==WM_LBUTTONDOWN || uMsg==WM_MBUTTONDOWN || uMsg==WM_RBUTTONDOWN)
	{
		if (sad::Input::inst()->areDownNotTracked() && sad::Input::inst()->areClickNotTracked())
			return 0;
		float mx=0,my=0,mz=0;
		int key=(wParam==MK_LBUTTON)?MOUSE_BUTTON_LEFT:(wParam==MK_RBUTTON)?MOUSE_BUTTON_RIGHT:(wParam==MK_MBUTTON)?MOUSE_BUTTON_MIDDLE:0;
		POINT pnt = ref()->_toClient(lParam);
		ref()->mapToOGL(pnt.x,pnt.y,mx,my,mz);
		sad::Input::inst()->postMouseDown(sad::Event(mx,my,mz,key));
		sad::Input::inst()->postMouseClick(sad::Event(mx,my,mz,key));
		return 0;
	}
	if (uMsg==WM_KEYDOWN || uMsg==WM_KEYUP)
	{
		sad::Event sev(0);
		SHORT c=GetAsyncKeyState(VK_CAPITAL)==1;
		SHORT a=GetAsyncKeyState(VK_MENU)<0;
		SHORT ct=GetAsyncKeyState(VK_CONTROL)<0;
		SHORT sh=GetAsyncKeyState(VK_SHIFT)<0;
		sev.capslock=c!=0;
		sev.alt=a!=0;
		sev.ctrl=ct!=0;
		sev.shift=sh!=0;
        if (table.contains(wParam))
		{
			sev.key=table[wParam];
			if (uMsg==WM_KEYDOWN)
			     sad::Input::inst()->postKeyDown(sev);
			else
				 sad::Input::inst()->postKeyUp(sev);
			return 0;
		}
		char af[5];
		GetKeyNameTextA(lParam,af,5);
		sev.key=af[0];
		if (uMsg==WM_KEYUP)
				sad::Input::inst()->postKeyDown(sev);
		else
			    sad::Input::inst()->postKeyUp(sev);
		return 0;
	}
	if (uMsg==WM_SIZE)
	{
		if (wParam==SIZE_MINIMIZED)
		{
			ref()->m_window.active=false;
		}
		else
		{
			ref()->m_window.active=true;
			ref()->reshape(LOWORD (lParam), HIWORD (lParam));
		}
	}
	return DefWindowProc (hWnd, uMsg, wParam, lParam);					// Pass Unhandled Messages To DefWindowProc
}


#endif
