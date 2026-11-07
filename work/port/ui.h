/* The port's screens over the game window (ui.cpp). */
#ifndef PORT_UI_H
#define PORT_UI_H

/* Hook the in-game menu, the FPS counter and the start screen into rsx_ui.
 * After the settings are loaded, before the rsx thread starts. argv is kept
 * for starting the process again with new settings. */
void port_ui_init(int argc, char** argv);

/* The rsx thread, once its backend is up: show the start screen if it is
 * wanted and the UI runs, else let the boot go on. */
void port_ui_backend_ready(void);

/* The main thread, before the guest boots: wait until the start screen's
 * "Play" (returns at once when there is none). */
void port_ui_wait_start(void);

/* Whether the start screen is showing (the boot keeps the window fresh at a
 * calmer pace then). */
int port_ui_start_screen(void);

/* PS3RECOMP_RT_ASPECT=auto and PS3RECOMP_ASPECT=auto (the Ultrawide option,
 * or play.sh) become the shape of the monitor the window opens on, as WxH,
 * or are removed -- 16:9 -- when it is not wider than 16:9 or no window
 * opens. The SPUs, the backend and the camera wrap each read these once and
 * have to agree, so this runs in main before any thread that reads the
 * environment starts. */
void port_resolve_aspect_auto(void);

#endif
