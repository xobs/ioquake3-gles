/*
 * Copyright (C) 2009  Nokia Corporation.  All rights reserved.
 */

#ifndef __EGL_INPUT_H__
#define __EGL_INPUT_H__

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

#include "../qcommon/q_shared.h"

extern cvar_t *in_nograb;
qboolean IN_MotionPressed(void);
void IN_ActivateMouse(void);
void IN_DeactivateMouse(void);
void IN_Frame(void);
void IN_Init(void);
void IN_Shutdown(void);
void IN_Restart(void);

#endif
