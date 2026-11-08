/* The player's settings: user/settings.ini (beside disc/, outside git), read
 * at start and turned into the environment variables the port already reads.
 * A variable already set wins over the file, so play.sh, the tests and the
 * benchmarks keep working as they did. The UI (launcher and in-game menu)
 * changes the values and saves them. */
#ifndef PORT_SETTINGS_H
#define PORT_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct port_settings {
    /* [video] */
    int scale;          /* render scale: 1 (720p), 2 (1440p), 3 (4K), 4 (5K) */
    int fullscreen;     /* 0 window, 1 full screen */
    int vsync;          /* 1 fifo, 0 mailbox */
    int stretch;        /* 1 stretch to the window, 0 black bars */
    int display;        /* which monitor the window opens on */
    /* The present chain (rsx_vulkan_backend.c, vk_post_record), changed live. */
    int post_aa;        /* 0 off, 1 FXAA, 2 SMAA 1x */
    int upscale;        /* a frame smaller than the window: 0 bilinear, 1 FSR 1 */
    int downscale;      /* a frame larger than the window: 0 bilinear, 1 area average */
    int sharpen;        /* RCAS in thousandths, 0..1000; -1 automatic */
    int fov;            /* field of view, percent wider than the game's (0..30): PS3RECOMP_FOV_SCALE */
    int ultrawide;      /* the picture in the monitor's shape, HUD in the middle 16:9:
                         * PS3RECOMP_RT_ASPECT and PS3RECOMP_ASPECT, both "auto" */
    /* [language] */
    char voice[24];     /* the voice pack: cache/psarc/<voice>.psarc */
    char text[24];      /* the text pack: cache/psarc/<text>_text.psarc */
    /* [performance] */
    int fps_limit;      /* 0 none, else frames a second */
    int fps_counter;    /* the counter drawn by the UI */
    int record_thread;  /* PS3RECOMP_BE_THREAD */
    int async_effects;  /* PS3RECOMP_VK_PIPE_SKIP: a draw whose new program is still being
                         * made is left out instead of waited for; changed live */
    /* [launcher] */
    int skip_launcher;  /* open the game without the start screen */
    /* [mods]: changes to how the game looks, beyond what the PS3 drew */
    int fullres_particles;  /* PS3RECOMP_FULLRES_PARTICLES */
    int aniso;              /* PS3RECOMP_ANISO: 0 (the game's own), 2, 4, 8, 16 */
    char mods_on[512];      /* the enabled mod folders, comma-separated, in load order */
    /* [keyboard]: keyboard and mouse as the pad (libs/input/pad_kbm.h), live */
    int kbm;                /* PS3RECOMP_KBM: 1 on, 0 off */
    int kbm_sens;           /* mouse sensitivity in hundredths, 10..500: PS3RECOMP_KBM_SENS */
    int kbm_invert_y;       /* PS3RECOMP_KBM_INVERT_Y */
    int kbm_deadzone;       /* the stick's starting deflection, percent 0..60: PS3RECOMP_KBM_DEADZONE */
    char kbm_map[1024];     /* "Key=ACTION ..." over the default layout: PS3RECOMP_KBM_MAP */
} port_settings;

/* The defaults: today's play.sh -- 720p, a window, mailbox, black bars,
 * Brazilian Portuguese voices and text, the recording thread on. */
void port_settings_defaults(port_settings* s);

/* Read `path` (missing or unreadable: the defaults). Returns 1 when the file
 * was read, 0 when the defaults were used. Unknown keys and bad values are
 * ignored, each falling back to its default. */
int port_settings_load(port_settings* s, const char* path);

/* Write `path`, creating its directory. 0 on success. A field pinned by the
 * environment (port_settings_from_env) is written as the file had it. */
int port_settings_save(const port_settings* s, const char* path);

/* The fields a variable already in the environment decides. */
enum {
    PORT_PIN_SCALE = 1 << 0, PORT_PIN_FULLSCREEN = 1 << 1, PORT_PIN_VSYNC = 1 << 2,
    PORT_PIN_STRETCH = 1 << 3, PORT_PIN_DISPLAY = 1 << 4, PORT_PIN_VOICE = 1 << 5,
    PORT_PIN_TEXT = 1 << 6, PORT_PIN_FPS_LIMIT = 1 << 7, PORT_PIN_RECORD_THREAD = 1 << 8,
    PORT_PIN_PARTICLES = 1 << 9, PORT_PIN_ANISO = 1 << 10, PORT_PIN_FOV = 1 << 11,
    PORT_PIN_POST_AA = 1 << 12, PORT_PIN_UPSCALE = 1 << 13, PORT_PIN_DOWNSCALE = 1 << 14,
    PORT_PIN_SHARPEN = 1 << 15, PORT_PIN_KBM = 1 << 16, PORT_PIN_KBM_SENS = 1 << 17,
    PORT_PIN_KBM_INVERT_Y = 1 << 18, PORT_PIN_KBM_DEADZONE = 1 << 19,
    PORT_PIN_ULTRAWIDE = 1 << 20, PORT_PIN_ASYNC_EFFECTS = 1 << 21,
    PORT_PIN_KBM_MAP = 1 << 22,
};

/* Before port_settings_export: each field whose variable is set takes the
 * environment's value, which is what the game will run with, so the UI shows
 * that. Returns the PORT_PIN_* bits (kept, see port_settings_pinned): the UI
 * greys those out and saving leaves the file's own values for them. */
unsigned port_settings_from_env(port_settings* s);
unsigned port_settings_pinned(void);

/* Each value as the environment variable the port reads, unless that
 * variable is already set. */
void port_settings_export(const port_settings* s);

/* The environment for starting the process again with new settings: this
 * one without what port_settings_export set (the new process reads the file
 * again), plus PS3_SKIP_LAUNCHER=1 and `extra` ("NAME=value") when not NULL.
 * Built aside rather than by unsetting: other threads read the environment,
 * and a failed exec keeps it whole. NULL-terminated, from malloc. */
char** port_settings_restart_env(const char* extra);

/* Player mods: the folders under mods/ (or $PS3_MODS_DIR), each with an
 * optional mod.ini (name, author, version, description) and files/ mirroring
 * the game's USRDIR. */
typedef struct port_mod {
    char dir[128];          /* folder name */
    char name[96];
    char author[64];
    char version[24];
    char description[256];
} port_mod;
/* Up to `cap`, sorted by folder name. Returns the count. */
int port_mods_scan(port_mod* out, int cap);
const char* port_mods_dir(void);
int port_mod_enabled(const port_settings* s, const char* dir);
void port_mod_set(port_settings* s, const char* dir, int on);

/* user/settings.ini, or $PS3_USER_DIR/settings.ini. */
const char* port_settings_path(void);

/* The process-wide settings, loaded once by main(). */
port_settings* port_settings_get(void);

#ifdef __cplusplus
}
#endif

#endif
