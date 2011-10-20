#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "pnd_event.h"
#include "egl_glimp.h"
#include "../client/client.h"
#include "../renderer/tr_local.h"
#include "../qcommon/q_shared.h"

static qboolean mouse_active = qfalse;
static qboolean mouse_avail = qfalse;

cvar_t *in_nograb;
static cvar_t *in_mouse;

void IN_ActivateMouse(void)
{
	if (!mouse_avail || !dpy || !win)
		return;

	if (!mouse_active) {
		if (!in_nograb->value)
			//install_grabs();
		mouse_active = qtrue;
	}
}

void IN_DeactivateMouse(void)
{
	if (!mouse_avail || !dpy || !win)
		return;

	if (mouse_active) {
		if (!in_nograb->value)
			//uninstall_grabs();
		mouse_active = qfalse;
	}
}

void IN_Frame(void)
{
	qboolean loading;

	//HandleEvents();
	PND_SendAllEvents();

	// If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading
	loading = !!(cls.state != CA_DISCONNECTED && cls.state != CA_ACTIVE);

	if (!r_fullscreen->integer && (Key_GetCatcher() & KEYCATCH_CONSOLE)) {
		// Console is down in windowed mode
		IN_DeactivateMouse();
	} else if (!r_fullscreen->integer && loading) {
		// Loading in windowed mode
		IN_DeactivateMouse();
	} else
		IN_ActivateMouse();
}

void IN_Init(void)
{
	Com_DPrintf("\n------- Input Initialization -------\n");

	PND_Setup_Controls();
	
	// mouse variables
	in_mouse = Cvar_Get("in_mouse", "1", CVAR_ARCHIVE);
	in_nograb = Cvar_Get("in_nograb", "0", CVAR_ARCHIVE);

	if (in_mouse->value) {
		mouse_avail = qtrue;
		IN_ActivateMouse();
	} else {
		IN_DeactivateMouse();
		mouse_avail = qfalse;
	}

	Com_DPrintf("------------------------------------\n");
}

void IN_Shutdown(void)
{
	IN_DeactivateMouse();
	mouse_avail = qfalse;
	
	PND_Close_Controls();
}

void IN_Restart(void)
{
	IN_Init();
}
