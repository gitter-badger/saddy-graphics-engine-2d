#include "renderer.h"
#include "texturemanager.h"
#include "log.h"


#ifdef WIN32

#define LOG_WRITE(MESG)    hst::log::inst()->owrite(hst::string(MESG))
#define UNIQUE_CLASS_NAME  "SADDY_RENDERER_WINDOW_INSTANCE_V_2"

void sad::Renderer::releaseWindow()
{
  if (m_window.fullscreen) //If fullscreen
  {
	  ChangeDisplaySettings(NULL,0);
	  ShowCursor(TRUE);
  }
  
  //Release rendering context
  if (m_window.hRC)
  {
	  if (!wglMakeCurrent(NULL,NULL)) 
		    LOG_WRITE("Renderer: Release of contexts failed\n");
	  if (!wglDeleteContext(m_window.hRC))     
		    LOG_WRITE("Renderer: Release rendering context failed\n");
	  m_window.hRC=NULL;
  }

  //Release device context
  if (m_window.hDC && !ReleaseDC(m_window.hWND,m_window.hDC))
  {
	   LOG_WRITE("Renderer: Releasing DC failed\n");
       m_window.hDC=NULL;
  }
  //If we can destroy the window
  if (m_window.hWND && !DestroyWindow(m_window.hWND))
  {
	  LOG_WRITE("Renderer: Releasing hWND failed\n");
      m_window.hWND=NULL;
  }
  
  //Unregister Class
  if (!UnregisterClass(UNIQUE_CLASS_NAME,m_window.hInstance))
  {
	  LOG_WRITE("Renderer: Could not unregister class\n");
	  m_window.hInstance=NULL;
  }

}

bool sad::Renderer::createWindow()
{
	//Window parameters
	WNDCLASS    wc;
    //Rectangle
	RECT        rect;
    rect.left  =(long)0;
	rect.right =(long)this->m_glsettings.width();
	rect.top   =(long)0;
	rect.bottom=(long)this->m_glsettings.height();


	//Init window
	m_window.hInstance = GetModuleHandle(NULL); 
    wc.style       = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC) sad::Renderer::WindowProc;
	wc.cbClsExtra  =  0;
	wc.cbWndExtra  =  0;
	wc.hInstance   =  m_window.hInstance;
	wc.hIcon       =  LoadIcon(NULL,IDI_WINLOGO);             // /TODO: Must be replaced in new releases (default icon)
    wc.hCursor     =  LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = UNIQUE_CLASS_NAME;

	if (!RegisterClass(&wc)) { LOG_WRITE("Renderer: Can't init  window\n"); return false;}
	
	//Setup fullscreen or windowed mode
	DWORD       ex_style=0;
	DWORD       style=0;
	this->adjustVideoMode(style,ex_style);
	AdjustWindowRectEx(&rect,style,FALSE,ex_style);

	//Create error
	m_window.hWND=CreateWindowExA(ex_style,UNIQUE_CLASS_NAME,m_windowtitle.data(),style,
		                         0,0,rect.right-rect.left,rect.bottom-rect.top,
								 NULL,NULL,m_window.hInstance,NULL);

	//Handle error
	if (!m_window.hWND)
	{
		releaseWindow();
		LOG_WRITE("Renderer: window creation error\n");
		return false;
	}
	//Getting a device context
	this->m_window.hDC=GetDC(m_window.hWND);
	if (!m_window.hDC)
	{
		releaseWindow();
		LOG_WRITE("Renderer: can't create a device context\n");
		return false;
	}
	if (!setupPFD())
	{
	    this->releaseWindow();
		LOG_WRITE("Renderer: can't setup pfd\n");
		return false;
	}
	//Creating context
	if (!(m_window.hRC=wglCreateContext(m_window.hDC)))
	{
		this->releaseWindow();
		LOG_WRITE("Renderer: can't create a rendering context\n");
		return false;
	}
	//Trying to activate a context
	if (!wglMakeCurrent(m_window.hDC,m_window.hRC))
	{
		this->releaseWindow();
		LOG_WRITE("Renderer: can't activate a rendering context\n");
		return false;
	}

	ShowWindow(m_window.hWND,SW_SHOW);
	SetForegroundWindow(m_window.hWND);
	SetFocus(m_window.hWND);
	reshape(m_glsettings.width(),m_glsettings.height());

	if (!initGLRendering())
	{
		this->releaseWindow();
		LOG_WRITE("Renderer: can't init GL rendering\n");
		return false;
	}

	return true;
}

void sad::Renderer::adjustVideoMode(unsigned long & style, unsigned long & ex_style)
{
	DEVMODEA & scr_settings=m_window.scr_settings;
	memset(&scr_settings,0,sizeof(scr_settings));
    scr_settings.dmSize       = sizeof(scr_settings);
    scr_settings.dmPelsWidth  = m_glsettings.width();
    scr_settings.dmPelsHeight = m_glsettings.height();
    scr_settings.dmBitsPerPel = 32;
    scr_settings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	if (m_window.fullscreen)
	{
		LONG result=ChangeDisplaySettings(&scr_settings,CDS_FULLSCREEN);
		if (result!=DISP_CHANGE_SUCCESSFUL)
		{
			LOG_WRITE("Renderer: changing to fullscreen failed, starting windowed\n");
			LOG_WRITE("Renderer: ChangeDisplaySettings failed with code ");
			hst::log::inst()->owrite(result);
			LOG_WRITE("\n");
			m_window.fullscreen=false;
		}
	}
	//If successful change
	if (m_window.fullscreen)
	{
		ex_style=WS_EX_APPWINDOW;
		style=WS_POPUP;
		ShowCursor(FALSE);
	}
	else
	{
		ex_style=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        style=WS_OVERLAPPEDWINDOW;
	}
}
bool sad::Renderer::setupPFD()
{
	static PIXELFORMATDESCRIPTOR pfd=
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
		16,
		0,0,0,0,0,0,
		0,              //No alpha buffer
		0,
		0,
		0,0,0,0,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0,0,0
	};

	GLuint      pixel_format=0; //Pixel format
	
	if (!(pixel_format=ChoosePixelFormat(m_window.hDC,&pfd)))
	{
		LOG_WRITE("Can't find a suitable PFD");
		return false;
	}
	if (!SetPixelFormat(m_window.hDC,pixel_format,&pfd))
	{
		LOG_WRITE("Can't set a PFD");
		return false;
	}

	return true;
}

void sad::Renderer::update()
{
 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 glMatrixMode(GL_MODELVIEW);
 glLoadIdentity();
 getCurrentScene()->render();
 SwapBuffers(m_window.hDC);
}

sad::Renderer::Renderer(void)
{
  m_windowtitle="SadExample";
  m_created=false;
  m_currentscene=NULL;
  this->m_window.hRC=NULL;
  this->m_window.hDC=NULL;
  this->m_window.hWND=NULL;
  this->m_window.hInstance=NULL;
}

void sad::Renderer::setWindowTitle(const hst::string & s)
{
	m_windowtitle=s;
	if (m_window.hWND)
         SetWindowTextA(m_window.hWND,s.data()); 
}
void sad::Renderer::quit()
{
	    PostMessage(m_window.hWND, WM_QUIT, 0, 0);							// Send A WM_QUIT Message
		m_running=false;
		m_created=false;
}

void sad::Renderer::toggleFullscreen()								// Toggle Fullscreen/Windowed
{
  if (m_running)
  {
   this->m_window.fullscreen=!this->m_window.fullscreen;
   LONG result;
   static RECT rct={0,0,0,0};
   if (this->m_window.fullscreen)
   {
		GetWindowRect(this->m_window.hWND,&rct);
		MoveWindow(m_window.hWND,0,0,rct.right-rct.left,rct.bottom-rct.top,TRUE);
		LONG style=GetWindowLong(m_window.hWND,GWL_STYLE);
		style &=~(WS_BORDER|WS_CAPTION|WS_THICKFRAME);
		SetWindowLong(m_window.hWND,GWL_STYLE,style);
		BOOL dp=SetWindowPos(this->m_window.hWND,NULL,rct.left,rct.top,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
	    if (!dp)
		{
			LOG_WRITE("Can't SetWindowPos with ");
			hst::log::inst()->owrite(GetLastError());
			LOG_WRITE("\n");
		}
		result=ChangeDisplaySettings(&(this->m_window.scr_settings),CDS_FULLSCREEN);	
   }
   else
   {
	    MoveWindow(m_window.hWND,rct.left,rct.top,rct.right-rct.left,rct.bottom-rct.top,TRUE);
	    result=ChangeDisplaySettings(NULL,0);
		LONG style=GetWindowLong(m_window.hWND,GWL_STYLE);
		style |=(WS_BORDER|WS_CAPTION|WS_THICKFRAME);
		SetWindowLong(m_window.hWND,GWL_STYLE,style);
		SetWindowPos(this->m_window.hWND,0,rct.left,rct.top,rct.right-rct.left,rct.bottom-rct.top,SWP_FRAMECHANGED);
   }
   if (result!=DISP_CHANGE_SUCCESSFUL)
   {
	   LOG_WRITE("Renderer: can't change mode\n");
   }
  }
  else
  {
	  this->m_window.fullscreen=!this->m_window.fullscreen;
  }
}


void sad::Renderer::toggleFixedOn()
{
	LONG style=GetWindowLongA(m_window.hWND,GWL_STYLE);
	style |= WS_OVERLAPPED;
    style &=  ~WS_THICKFRAME;	
	LONG result=SetWindowLongA(m_window.hWND,GWL_STYLE,style);
}
void sad::Renderer::toggleFixedOff()
{
	LONG style=GetWindowLongA(m_window.hWND,GWL_STYLE);
	style |=WS_OVERLAPPED;
	style |= WS_THICKFRAME;
	SetWindowLongA(m_window.hWND,GWL_STYLE,style);
}
#endif
