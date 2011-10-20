#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include "egl_glimp.h"
#include "../client/client.h"
#include "../renderer/tr_local.h"

Display *dpy = NULL;
Window win = 0;
EGLContext eglContext = NULL;
EGLDisplay eglDisplay = NULL;
EGLSurface eglSurface = NULL;

int pandora_driver_mode_x11 = 0;
cvar_t* cvarPndMode;

int Sys_XTimeToSysTime(Time xtime)
{
	return Sys_Milliseconds();
}

static char *GLimp_StringErrors[] = {
	"EGL_SUCCESS",
	"EGL_NOT_INITIALIZED",
	"EGL_BAD_ACCESS",
	"EGL_BAD_ALLOC",
	"EGL_BAD_ATTRIBUTE",
	"EGL_BAD_CONFIG",
	"EGL_BAD_CONTEXT",
	"EGL_BAD_CURRENT_SURFACE",
	"EGL_BAD_DISPLAY",
	"EGL_BAD_MATCH",
	"EGL_BAD_NATIVE_PIXMAP",
	"EGL_BAD_NATIVE_WINDOW",
	"EGL_BAD_PARAMETER",
	"EGL_BAD_SURFACE",
	"EGL_CONTEXT_LOST",
};

static void GLimp_HandleError(void)
{
	GLint err = eglGetError();

	fprintf(stderr, "%s: 0x%04x: %s\n", __func__, err,
		GLimp_StringErrors[err]);
	assert(0);
}

#define _NET_WM_STATE_REMOVE        0	/* remove/unset property */
#define _NET_WM_STATE_ADD           1	/* add/set property */
#define _NET_WM_STATE_TOGGLE        2	/* toggle property  */

static void GLimp_DisableComposition(void)
{
	XClientMessageEvent xclient;
	Atom atom;
	int one = 1;

	atom = XInternAtom(dpy, "_HILDON_NON_COMPOSITED_WINDOW", False);
	XChangeProperty(dpy, win, atom, XA_INTEGER, 32, PropModeReplace,
			(unsigned char *)&one, 1);

	xclient.type = ClientMessage;
	xclient.window = win;	//GDK_WINDOW_XID (window);
	xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
	xclient.format = 32;
	xclient.data.l[0] =
	    r_fullscreen->integer ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
	//gdk_x11_atom_to_xatom_for_display (display, state1);
	//gdk_x11_atom_to_xatom_for_display (display, state2);
	xclient.data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	xclient.data.l[2] = 0;
	xclient.data.l[3] = 0;
	xclient.data.l[4] = 0;
	XSendEvent(dpy, DefaultRootWindow(dpy), False,
		   SubstructureRedirectMask | SubstructureNotifyMask,
		   (XEvent *) & xclient);
}


#define MAX_NUM_CONFIGS 4

/*
 * Create an RGB, double-buffered window.
 * Return the window and context handles.
 */
static void make_window(Display * dpy, Screen * scr, EGLDisplay eglDisplay,
			EGLSurface * winRet, EGLContext * ctxRet)
{
	EGLSurface eglSurface = EGL_NO_SURFACE;
	EGLContext eglContext;
	EGLConfig configs[MAX_NUM_CONFIGS];
	EGLint config_count;
	XWindowAttributes WinAttr;
	int XResult = BadImplementation;
	int blackColour;
	EGLint cfg_attribs[] = {
		/* RGB565 */
		EGL_BUFFER_SIZE, 16,
		EGL_RED_SIZE, 5,
		EGL_GREEN_SIZE, 6,
		EGL_BLUE_SIZE, 5,

		EGL_DEPTH_SIZE, 8,

		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,

		EGL_NONE
	};
	EGLint i;

	if (pandora_driver_mode_x11)
	{
		blackColour = BlackPixel(dpy, DefaultScreen(dpy));
		win =
		    XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0,
					blackColour, blackColour);
		XStoreName(dpy, win, WINDOW_CLASS_NAME);

		XSelectInput(dpy, win, X_MASK);

		if (!(XResult = XGetWindowAttributes(dpy, win, &WinAttr)))
			GLimp_HandleError();

		GLimp_DisableComposition();
		XMapWindow(dpy, win);
		GLimp_DisableComposition();

		XFlush(dpy);
	}

	if (!eglGetConfigs(eglDisplay, configs, MAX_NUM_CONFIGS, &config_count))
		GLimp_HandleError();

	if (!eglChooseConfig
	    (eglDisplay, cfg_attribs, configs, MAX_NUM_CONFIGS, &config_count))
		GLimp_HandleError();

	for (i = 0; i < config_count; i++)
	{
		if (pandora_driver_mode_x11)
		{
			if ((eglSurface =
			    eglCreateWindowSurface(eglDisplay, configs[i],
						    (NativeDisplayType) win,
						    NULL)) != EGL_NO_SURFACE)
				break;
		}
		else
		{
			if ((eglSurface =
			    eglCreateWindowSurface( eglDisplay, configs[i],
						    (NativeDisplayType)NULL,
						    NULL)) != EGL_NO_SURFACE)
				break;
		}
	}
	if (eglSurface == EGL_NO_SURFACE)
		GLimp_HandleError();

	if ((eglContext =
	     eglCreateContext(eglDisplay, configs[i], EGL_NO_CONTEXT,
			      NULL)) == EGL_NO_CONTEXT)
		GLimp_HandleError();

	if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
		GLimp_HandleError();

	*winRet = eglSurface;
	*ctxRet = eglContext;
}

static qboolean GLimp_HaveExtension(const char *ext)
{
	const char *ptr = Q_stristr( glConfig.extensions_string, ext );
	if (ptr == NULL)
		return qfalse;
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
}

static void qglMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t)
{
	qglMultiTexCoord4f(target,s,t,1,1);
}


/*
===============
GLimp_InitExtensions
===============
*/
static void GLimp_InitExtensions( void )
{
	if ( !r_allowExtensions->integer )
	{
		ri.Printf( PRINT_ALL, "* IGNORING OPENGL EXTENSIONS *\n" );
		return;
	}

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	glConfig.textureCompression = TC_NONE;

	// GL_EXT_texture_compression_s3tc
	if ( GLimp_HaveExtension( "GL_ARB_texture_compression" ) &&
	     GLimp_HaveExtension( "GL_EXT_texture_compression_s3tc" ) )
	{
		if ( r_ext_compressed_textures->value )
		{
			glConfig.textureCompression = TC_S3TC_ARB;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc not found\n" );
	}

	// GL_S3_s3tc ... legacy extension before GL_EXT_texture_compression_s3tc.
	if (glConfig.textureCompression == TC_NONE)
	{
		if ( GLimp_HaveExtension( "GL_S3_s3tc" ) )
		{
			if ( r_ext_compressed_textures->value )
			{
				glConfig.textureCompression = TC_S3TC;
				ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
			}
			else
			{
				ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...GL_S3_s3tc not found\n" );
		}
	}


	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qtrue; //qfalse;

	// GL_ARB_multitexture
	/*
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	*/
	//if ( GLimp_HaveExtension( "GL_ARB_multitexture" ) )
	{
		if ( r_ext_multitexture->value )
		{
			qglMultiTexCoord2fARB = qglMultiTexCoord2f;
			qglActiveTextureARB = qglActiveTexture;
			qglClientActiveTextureARB = qglClientActiveTexture;

			if ( qglActiveTextureARB )
			{
				GLint glint = 0;
				qglGetIntegerv( GL_MAX_TEXTURE_UNITS, &glint );
				glConfig.numTextureUnits = (int) glint;
				if ( glConfig.numTextureUnits > 1 )
				{
					ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
				}
				else
				{
					qglMultiTexCoord2fARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
				}
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
		}
	}

	{
		ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
	}

	textureFilterAnisotropic = qfalse;

	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
	}
}

void GLimp_Init(void)
{
	Screen *screen = NULL;
	Visual *vis;
	EGLint major, minor;

	ri.Printf(PRINT_ALL, "Initializing OpenGL subsystem\n");

	cvarPndMode = ri.Cvar_Get("x11", "0", 0);
	pandora_driver_mode_x11 = cvarPndMode->value;
	
	bzero(&glConfig, sizeof(glConfig));

	if (pandora_driver_mode_x11)
	{
		if (!(dpy = XOpenDisplay(NULL))) {
			printf("Error: couldn't open display \n");
			assert(0);
		}
		screen = XDefaultScreenOfDisplay(dpy);
		vis = DefaultVisual(dpy, DefaultScreen(dpy));

		eglDisplay = eglGetDisplay((NativeDisplayType) dpy);
	}
	else
	{
		dpy = NULL;
		eglDisplay = eglGetDisplay((NativeDisplayType)NULL );
	}
	
		if (!eglInitialize(eglDisplay, &major, &minor))
			GLimp_HandleError();	
	
	make_window(dpy, screen, eglDisplay, &eglSurface, &eglContext);

	if (pandora_driver_mode_x11)
	{
		XMoveResizeWindow(dpy, win, 0, 0, WidthOfScreen(screen),
				HeightOfScreen(screen));
		glConfig.vidWidth = WidthOfScreen(screen);
		glConfig.vidHeight = HeightOfScreen(screen);
	}
	else
	{
		glConfig.vidWidth  = 800;
		glConfig.vidHeight = 480;
	}
	
	glConfig.isFullscreen = r_fullscreen->integer;	
	glConfig.windowAspect = (float)glConfig.vidWidth / glConfig.vidHeight;
	// FIXME
	//glConfig.colorBits = 0
	//glConfig.stencilBits = 0;
	//glConfig.depthBits = 0;
	glConfig.textureCompression = TC_NONE;

	// This values force the UI to disable driver selection
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;

	Q_strncpyz(glConfig.vendor_string,
		   (const char *)qglGetString(GL_VENDOR),
		   sizeof(glConfig.vendor_string));
	Q_strncpyz(glConfig.renderer_string,
		   (const char *)qglGetString(GL_RENDERER),
		   sizeof(glConfig.renderer_string));
	Q_strncpyz(glConfig.version_string,
		   (const char *)qglGetString(GL_VERSION),
		   sizeof(glConfig.version_string));
	Q_strncpyz(glConfig.extensions_string,
		   (const char *)qglGetString(GL_EXTENSIONS),
		   sizeof(glConfig.extensions_string));

	qglLockArraysEXT = qglLockArrays;
	qglUnlockArraysEXT = qglUnlockArrays;

	GLimp_InitExtensions();

	IN_Init( );

	ri.Printf(PRINT_ALL, "------------------\n");
}

void GLimp_LogComment(char *comment)
{
	//fprintf(stderr, "%s: %s\n", __func__, comment);
}

void GLimp_EndFrame(void)
{
	if (Q_stricmp(r_drawBuffer->string, "GL_FRONT") != 0) {
		eglSwapBuffers(eglDisplay, eglSurface);
	}

	if (pandora_driver_mode_x11)
		XForceScreenSaver(dpy, ScreenSaverReset);

}

void GLimp_Shutdown(void)
{
	IN_Shutdown();

	eglDestroyContext(eglDisplay, eglContext);
	eglDestroySurface(eglDisplay, eglSurface);
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglTerminate(eglDisplay);

	if (pandora_driver_mode_x11)
	{
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
}

#if 1
void qglArrayElement(GLint i)
{
}

void qglCallList(GLuint list)
{
}

void qglDrawBuffer(GLenum mode)
{
}

void qglLockArrays(GLint i, GLsizei size)
{
}

void qglUnlockArrays(void)
{
}
#endif

#if 1
void GLimp_SetGamma(unsigned char red[256], unsigned char green[256],
		    unsigned char blue[256])
{
}

qboolean GLimp_SpawnRenderThread(void (*function) (void))
{
	return qfalse;
}

void GLimp_FrontEndSleep(void)
{
}

void *GLimp_RendererSleep(void)
{
	return NULL;
}

void GLimp_RenderThreadWrapper(void *data)
{
}

void GLimp_WakeRenderer(void *data)
{
}
#endif
