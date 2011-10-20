/*
 * Copyright (C) 2009  Nokia Corporation.  All rights reserved.
 */

#ifndef __EGL_GLIMP_H__
#define __EGL_GLIMP_H__

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <GLES/egl.h>
#include <GLES/gl.h>

#include "egl_input.h"

#ifndef GLAPI
#define GLAPI extern
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifndef APIENTRY
#define APIENTRY GLAPIENTRY
#endif

/* "P" suffix to be used for a pointer to a function */
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

#ifndef GLAPIENTRYP
#define GLAPIENTRYP GLAPIENTRY *
#endif

extern Display *dpy;
extern Window win;
extern EGLContext eglContext;
extern EGLDisplay eglDisplay;
extern EGLSurface eglSurface;
int Sys_XTimeToSysTime(Time xtime);
void GLimp_Init(void);
void GLimp_LogComment(char *comment);
void GLimp_EndFrame(void);
void GLimp_Shutdown(void);
void qglArrayElement(GLint i);
void qglCallList(GLuint list);
void qglDrawBuffer(GLenum mode);
void qglLockArrays(GLint i, GLsizei size);
void qglUnlockArrays(void);
void GLimp_SetGamma(unsigned char red[256], unsigned char green[256],unsigned char blue[256]);
//qboolean GLimp_SpawnRenderThread(void (*function) (void));
void GLimp_FrontEndSleep(void);
void *GLimp_RendererSleep(void);
void GLimp_RenderThreadWrapper(void *data);
void GLimp_WakeRenderer(void *data);

#define WINDOW_CLASS_NAME	"Quake III: Arena"

#define KEY_MASK	(KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK	(ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask)
#define X_MASK		(KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask)


#endif
