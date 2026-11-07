/* settings.cpp: round trip, defaults, bad values, the environment winning,
 * pinned fields, the restart environment.
 *   c++ -std=c++17 -I.. ../settings.cpp settings_test.cpp -o /tmp/st && /tmp/st */
#include "settings.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main()
{
    char dir[] = "/tmp/settings_test.XXXXXX";
    if (!mkdtemp(dir)) return 2;
    const std::string path = std::string(dir) + "/sub/settings.ini";

    port_settings s, r;
    CHECK(port_settings_load(&s, path.c_str()) == 0);          /* missing: defaults */
    CHECK(s.scale == 1 && s.record_thread == 1 && !strcmp(s.voice, "brazilian"));

    s.scale = 3; s.fullscreen = 1; s.vsync = 1; s.stretch = 1; s.display = 2; s.fov = 15;
    snprintf(s.voice, sizeof s.voice, "english"); snprintf(s.text, sizeof s.text, "brazilian");
    s.fps_limit = 60; s.fps_counter = 1; s.record_thread = 0; s.skip_launcher = 1;
    CHECK(port_settings_save(&s, path.c_str()) == 0);            /* creates sub/ */
    CHECK(port_settings_load(&r, path.c_str()) == 1);
    CHECK(!memcmp(&s, &r, sizeof s));

    /* bad values fall back, unknown keys and sections are ignored */
    FILE* f = fopen(path.c_str(), "w");
    fputs("[video]\nscale = 9\nvsync = yes\n[language]\nvoice = klingon\ntext = french ; c\n"
          "[nope]\nx = 1\ngarbage line\n[performance]\nfps_limit = 120\n", f);
    fclose(f);
    CHECK(port_settings_load(&r, path.c_str()) == 1);
    CHECK(r.scale == 1 && r.vsync == 0 && !strcmp(r.voice, "brazilian") && !strcmp(r.text, "french"));
    CHECK(r.fps_limit == 120);

    /* export: the environment wins; the console language follows the text */
    unsetenv("PS3RECOMP_RES_SCALE"); unsetenv("PS3_SYS_LANG"); setenv("PS3_VOICE_LANG", "japanese", 1);
    r.scale = 2;
    port_settings_export(&r);
    CHECK(!strcmp(getenv("PS3RECOMP_RES_SCALE"), "2"));
    CHECK(!strcmp(getenv("PS3_VOICE_LANG"), "japanese"));
    CHECK(!strcmp(getenv("PS3_TEXT_LANG"), "french"));
    CHECK(!strcmp(getenv("PS3_SYS_LANG"), "fr"));

    /* the environment pins what it sets: the struct shows it, the file keeps its own */
    {
        const std::string p2 = std::string(dir) + "/pin.ini";
        port_settings a; port_settings_defaults(&a);
        a.scale = 1; snprintf(a.voice, sizeof a.voice, "english");
        CHECK(port_settings_save(&a, p2.c_str()) == 0);
        setenv("PS3RECOMP_RES_SCALE", "3", 1); unsetenv("PS3_VOICE_LANG"); unsetenv("PS3_TEXT_LANG");
        unsetenv("PS3RECOMP_VK_FULLSCREEN"); unsetenv("PS3RECOMP_VK_PRESENT"); unsetenv("PS3RECOMP_VK_STRETCH");
        unsetenv("PS3RECOMP_VK_DISPLAY"); unsetenv("PS3RECOMP_FPS_LIMIT"); setenv("PS3RECOMP_BE_THREAD", "0", 1);
        unsetenv("PS3RECOMP_FULLRES_PARTICLES"); unsetenv("PS3RECOMP_ANISO"); unsetenv("PS3RECOMP_FOV_SCALE");
        unsetenv("PS3RECOMP_POST_AA"); unsetenv("PS3RECOMP_UPSCALE");
        unsetenv("PS3RECOMP_DOWNSCALE"); unsetenv("PS3RECOMP_SHARPEN");
        unsetenv("PS3RECOMP_KBM"); unsetenv("PS3RECOMP_KBM_SENS");
        unsetenv("PS3RECOMP_KBM_INVERT_Y"); unsetenv("PS3RECOMP_KBM_DEADZONE");
        port_settings b;
        CHECK(port_settings_load(&b, p2.c_str()) == 1);
        const unsigned pin = port_settings_from_env(&b);
        CHECK(pin == (PORT_PIN_SCALE | PORT_PIN_RECORD_THREAD));
        CHECK(port_settings_pinned() == pin);
        CHECK(b.scale == 3 && b.record_thread == 0 && !strcmp(b.voice, "english"));
        b.fps_counter = 1;                                    /* the UI changes something else */
        CHECK(port_settings_save(&b, p2.c_str()) == 0);
        port_settings c;
        CHECK(port_settings_load(&c, p2.c_str()) == 1);
        CHECK(c.scale == 1 && c.record_thread == 1 && c.fps_counter == 1);
        unsetenv("PS3RECOMP_RES_SCALE"); unsetenv("PS3RECOMP_BE_THREAD");
        port_settings_from_env(&c);                           /* nothing pinned again */
        CHECK(port_settings_pinned() == 0);
        remove(p2.c_str());
    }

    /* mods: saved, read back, clamped */
    {
        const std::string p3 = std::string(dir) + "/mods.ini";
        port_settings a; port_settings_defaults(&a);
        CHECK(a.fullres_particles == 1 && a.aniso == 16);
        a.fullres_particles = 0; a.aniso = 4;
        CHECK(port_settings_save(&a, p3.c_str()) == 0);
        port_settings b;
        CHECK(port_settings_load(&b, p3.c_str()) == 1);
        CHECK(b.fullres_particles == 0 && b.aniso == 4);
        FILE* g = fopen(p3.c_str(), "w");
        fputs("[mods]\naniso = 64\nfullres_particles = 7\n", g);
        fclose(g);
        CHECK(port_settings_load(&b, p3.c_str()) == 1);
        CHECK(b.aniso == 16 && b.fullres_particles == 1);   /* out of range: the defaults */
        remove(p3.c_str());
    }

    /* mod list: toggled in order, read back, exported only when on disk */
    {
        port_settings a; port_settings_defaults(&a);
        port_mod_set(&a, "alpha", 1); port_mod_set(&a, "beta", 1); port_mod_set(&a, "alpha", 0);
        port_mod_set(&a, "alpha", 1);
        CHECK(!strcmp(a.mods_on, "beta,alpha"));
        CHECK(port_mod_enabled(&a, "alpha") && port_mod_enabled(&a, "beta") && !port_mod_enabled(&a, "alp"));
        const std::string p4 = std::string(dir) + "/modlist.ini";
        CHECK(port_settings_save(&a, p4.c_str()) == 0);
        port_settings b;
        CHECK(port_settings_load(&b, p4.c_str()) == 1);
        CHECK(!strcmp(b.mods_on, "beta,alpha"));
        remove(p4.c_str());
    }

    /* voices: text-only languages are refused */
    f = fopen(path.c_str(), "w");
    fputs("\xEF\xBB\xBF[language]\nvoice = korean\ntext = korean\n", f);   /* with a BOM */
    fclose(f);
    CHECK(port_settings_load(&r, path.c_str()) == 1);
    CHECK(!strcmp(r.voice, "brazilian") && !strcmp(r.text, "korean"));

    /* the restart environment: without what export set, with the skip */
    {
        unsetenv("PS3RECOMP_RES_SCALE"); unsetenv("PS3_SKIP_LAUNCHER");
        setenv("SETTINGS_TEST_KEEP", "1", 1); setenv("PS3_VOICE_LANG", "english", 1);
        port_settings e; port_settings_defaults(&e);
        port_settings_export(&e);                             /* RES_SCALE is ours now */
        char** env = port_settings_restart_env("SETTINGS_TEST_EXTRA=1");
        CHECK(env != nullptr);
        bool scale = false, keep = false, skip = false, voice = false, extra = false;
        for (char** p = env; p && *p; p++) {
            if (!strncmp(*p, "PS3RECOMP_RES_SCALE=", 20)) scale = true;
            if (!strcmp(*p, "SETTINGS_TEST_KEEP=1")) keep = true;
            if (!strcmp(*p, "PS3_SKIP_LAUNCHER=1")) skip = true;
            if (!strncmp(*p, "PS3_VOICE_LANG=", 15)) voice = true;   /* the caller's, not ours */
            if (!strcmp(*p, "SETTINGS_TEST_EXTRA=1")) extra = true;
        }
        CHECK(!scale && keep && skip && voice && extra);
        CHECK(getenv("PS3RECOMP_RES_SCALE") != nullptr);      /* this process keeps its own */
        free(env);
    }

    /* the present chain: names and the sharpening fraction, both ways. Last:
     * its export leaves variables set that the checks above count. */
    {
        const std::string p5 = std::string(dir) + "/post.ini";
        port_settings a; port_settings_defaults(&a);
        CHECK(a.post_aa == 0 && a.upscale == 0 && a.downscale == 0 && a.sharpen == -1);
        a.scale = 4; a.post_aa = 2; a.upscale = 1; a.downscale = 1; a.sharpen = 350;
        CHECK(port_settings_save(&a, p5.c_str()) == 0);
        port_settings b;
        CHECK(port_settings_load(&b, p5.c_str()) == 1);
        CHECK(!memcmp(&a, &b, sizeof a));
        FILE* g = fopen(p5.c_str(), "w");
        fputs("[video]\naa = msaa\nupscale = fsr1\ndownscale = lanczos\nsharpen = 1.5\n", g);
        fclose(g);
        CHECK(port_settings_load(&b, p5.c_str()) == 1);
        CHECK(b.post_aa == 0 && b.upscale == 1 && b.downscale == 0 && b.sharpen == -1);
        g = fopen(p5.c_str(), "w");
        fputs("[video]\naa = fxaa\nsharpen = 0\n", g);
        fclose(g);
        CHECK(port_settings_load(&b, p5.c_str()) == 1);
        CHECK(b.post_aa == 1 && b.sharpen == 0);
        unsetenv("PS3RECOMP_POST_AA"); unsetenv("PS3RECOMP_UPSCALE");
        unsetenv("PS3RECOMP_DOWNSCALE"); unsetenv("PS3RECOMP_SHARPEN");
        b.post_aa = 2; b.upscale = 1; b.downscale = 0; b.sharpen = -1;
        port_settings_export(&b);
        CHECK(!strcmp(getenv("PS3RECOMP_POST_AA"), "smaa") && !strcmp(getenv("PS3RECOMP_UPSCALE"), "fsr1"));
        CHECK(!strcmp(getenv("PS3RECOMP_DOWNSCALE"), "bilinear") && !strcmp(getenv("PS3RECOMP_SHARPEN"), "auto"));
        setenv("PS3RECOMP_SHARPEN", "0.25", 1);
        port_settings c; port_settings_defaults(&c);
        CHECK((port_settings_from_env(&c) & (PORT_PIN_POST_AA | PORT_PIN_SHARPEN)) ==
              (PORT_PIN_POST_AA | PORT_PIN_SHARPEN));
        CHECK(c.post_aa == 2 && c.sharpen == 250);
        unsetenv("PS3RECOMP_POST_AA"); unsetenv("PS3RECOMP_UPSCALE");
        unsetenv("PS3RECOMP_DOWNSCALE"); unsetenv("PS3RECOMP_SHARPEN");
        remove(p5.c_str());
    }
    /* keyboard and mouse: round trip, clamping, the environment pinning,
     * and the export cellPad reads */
    {
        const std::string p6 = std::string(dir) + "/kbm.ini";
        /* The exports above set these: clear them, and the pins they made. */
        unsetenv("PS3RECOMP_KBM"); unsetenv("PS3RECOMP_KBM_SENS");
        unsetenv("PS3RECOMP_KBM_INVERT_Y"); unsetenv("PS3RECOMP_KBM_DEADZONE"); unsetenv("PS3RECOMP_KBM_MAP");
        port_settings a; port_settings_defaults(&a);
        port_settings_from_env(&a);
        port_settings_defaults(&a);
        CHECK(a.kbm == 1 && a.kbm_sens == 100 && a.kbm_invert_y == 0 && a.kbm_deadzone == 20 && !a.kbm_map[0]);
        a.kbm = 0; a.kbm_sens = 235; a.kbm_invert_y = 1; a.kbm_deadzone = 5;
        snprintf(a.kbm_map, sizeof a.kbm_map, "F=SQUARE, Q=NONE G=R2");
        CHECK(port_settings_save(&a, p6.c_str()) == 0);
        port_settings b;
        CHECK(port_settings_load(&b, p6.c_str()) == 1);
        CHECK(b.kbm == 0 && b.kbm_sens == 235 && b.kbm_invert_y == 1 && b.kbm_deadzone == 5);
        CHECK(!strcmp(b.kbm_map, "F=SQUARE, Q=NONE G=R2"));
        FILE* g = fopen(p6.c_str(), "w");
        fputs("[keyboard]\nenabled = 2\nsensitivity = 9\ndeadzone = 0.7\ninvert_y = 1\n", g);
        fclose(g);
        CHECK(port_settings_load(&b, p6.c_str()) == 1);
        CHECK(b.kbm == 1 && b.kbm_sens == 100 && b.kbm_deadzone == 20 && b.kbm_invert_y == 1);
        g = fopen(p6.c_str(), "w");
        fputs("[keyboard]\nsensitivity = 0.5\ndeadzone = 0.15\n", g);
        fclose(g);
        CHECK(port_settings_load(&b, p6.c_str()) == 1 && b.kbm_sens == 50 && b.kbm_deadzone == 15);
        setenv("PS3RECOMP_KBM", "0", 1); setenv("PS3RECOMP_KBM_SENS", "1.5", 1);
        const unsigned pin = port_settings_from_env(&b);
        CHECK((pin & PORT_PIN_KBM) && (pin & PORT_PIN_KBM_SENS) && !(pin & PORT_PIN_KBM_DEADZONE));
        CHECK(b.kbm == 0 && b.kbm_sens == 150);
        CHECK(port_settings_save(&b, p6.c_str()) == 0);       /* pinned: the file keeps its own */
        port_settings c;
        CHECK(port_settings_load(&c, p6.c_str()) == 1 && c.kbm == 1 && c.kbm_sens == 50);
        unsetenv("PS3RECOMP_KBM"); unsetenv("PS3RECOMP_KBM_SENS");
        unsetenv("PS3RECOMP_KBM_INVERT_Y"); unsetenv("PS3RECOMP_KBM_DEADZONE"); unsetenv("PS3RECOMP_KBM_MAP");
        port_settings_from_env(&c);
        c.kbm_invert_y = 1;
        snprintf(c.kbm_map, sizeof c.kbm_map, "G=R2");
        port_settings_export(&c);
        CHECK(getenv("PS3RECOMP_KBM") && !strcmp(getenv("PS3RECOMP_KBM"), "1"));
        CHECK(getenv("PS3RECOMP_KBM_SENS") && !strcmp(getenv("PS3RECOMP_KBM_SENS"), "0.50"));
        CHECK(getenv("PS3RECOMP_KBM_INVERT_Y") && !strcmp(getenv("PS3RECOMP_KBM_INVERT_Y"), "1"));
        CHECK(getenv("PS3RECOMP_KBM_DEADZONE") && !strcmp(getenv("PS3RECOMP_KBM_DEADZONE"), "0.15"));
        CHECK(getenv("PS3RECOMP_KBM_MAP") && !strcmp(getenv("PS3RECOMP_KBM_MAP"), "G=R2"));
        unsetenv("PS3RECOMP_KBM"); unsetenv("PS3RECOMP_KBM_SENS");
        unsetenv("PS3RECOMP_KBM_INVERT_Y"); unsetenv("PS3RECOMP_KBM_DEADZONE"); unsetenv("PS3RECOMP_KBM_MAP");
        remove(p6.c_str());
    }
    /* field of view: clamped in the file, the scale's percent from the
     * environment, exported only when it widens anything (last: export
     * remembers what it set, which the restart check above relies on) */
    {
        const std::string p5 = std::string(dir) + "/fov.ini";
        FILE* g = fopen(p5.c_str(), "w");
        fputs("[video]\nfov = 45\n", g);
        fclose(g);
        port_settings a;
        CHECK(port_settings_load(&a, p5.c_str()) == 1 && a.fov == 0);
        g = fopen(p5.c_str(), "w");
        fputs("[video]\nfov = 20\n", g);
        fclose(g);
        CHECK(port_settings_load(&a, p5.c_str()) == 1 && a.fov == 20);
        setenv("PS3RECOMP_FOV_SCALE", "1.1", 1);
        CHECK((port_settings_from_env(&a) & PORT_PIN_FOV) && a.fov == 10);
        CHECK(port_settings_save(&a, p5.c_str()) == 0);       /* pinned: the file keeps 20 */
        port_settings b;
        CHECK(port_settings_load(&b, p5.c_str()) == 1 && b.fov == 20);
        unsetenv("PS3RECOMP_FOV_SCALE");
        port_settings_from_env(&b);
        b.fov = 0;
        port_settings_export(&b);
        CHECK(getenv("PS3RECOMP_FOV_SCALE") == nullptr);
        b.fov = 25;
        port_settings_export(&b);
        CHECK(getenv("PS3RECOMP_FOV_SCALE") && !strcmp(getenv("PS3RECOMP_FOV_SCALE"), "1.25"));
        unsetenv("PS3RECOMP_FOV_SCALE");
        remove(p5.c_str());
    }

    /* ultrawide: round trip, a shape in the environment pinning it, both
     * variables exported as "auto" only when it is on */
    {
        const std::string p7 = std::string(dir) + "/uw.ini";
        port_settings a; port_settings_defaults(&a);
        CHECK(a.ultrawide == 0);
        a.ultrawide = 1;
        CHECK(port_settings_save(&a, p7.c_str()) == 0);
        port_settings b;
        CHECK(port_settings_load(&b, p7.c_str()) == 1 && b.ultrawide == 1);
        FILE* g = fopen(p7.c_str(), "w");
        fputs("[video]\nultrawide = 3\n", g);
        fclose(g);
        CHECK(port_settings_load(&b, p7.c_str()) == 1 && b.ultrawide == 0);
        const char* const wide[] = { "auto", "3440x1440", "21:9", "2.389", "5120/1440" };
        const char* const narrow[] = { "16:9", "1920x1080", "16:10", "1.78", "junk" };
        for (const char* v : wide) {
            setenv("PS3RECOMP_RT_ASPECT", v, 1); b.ultrawide = 0;
            CHECK((port_settings_from_env(&b) & PORT_PIN_ULTRAWIDE) && b.ultrawide == 1);
        }
        for (const char* v : narrow) {
            setenv("PS3RECOMP_RT_ASPECT", v, 1); b.ultrawide = 1;
            CHECK((port_settings_from_env(&b) & PORT_PIN_ULTRAWIDE) && b.ultrawide == 0);
        }
        g = fopen(p7.c_str(), "w");
        fputs("[video]\nultrawide = 0\n", g);
        fclose(g);
        CHECK(port_settings_load(&b, p7.c_str()) == 1);
        setenv("PS3RECOMP_RT_ASPECT", "3440x1440", 1);
        CHECK((port_settings_from_env(&b) & PORT_PIN_ULTRAWIDE) && b.ultrawide == 1);
        CHECK(port_settings_save(&b, p7.c_str()) == 0);       /* pinned: the file keeps 0 */
        port_settings c;
        CHECK(port_settings_load(&c, p7.c_str()) == 1 && c.ultrawide == 0);
        unsetenv("PS3RECOMP_RT_ASPECT"); unsetenv("PS3RECOMP_ASPECT");
        CHECK(!(port_settings_from_env(&c) & PORT_PIN_ULTRAWIDE));
        port_settings_export(&c);
        CHECK(!getenv("PS3RECOMP_RT_ASPECT") && !getenv("PS3RECOMP_ASPECT"));
        c.ultrawide = 1;
        port_settings_export(&c);
        CHECK(getenv("PS3RECOMP_RT_ASPECT") && !strcmp(getenv("PS3RECOMP_RT_ASPECT"), "auto"));
        CHECK(getenv("PS3RECOMP_ASPECT") && !strcmp(getenv("PS3RECOMP_ASPECT"), "auto"));
        unsetenv("PS3RECOMP_RT_ASPECT"); unsetenv("PS3RECOMP_ASPECT");
        setenv("PS3RECOMP_ASPECT", "2.0", 1);                 /* the environment's own wins */
        port_settings_export(&c);
        CHECK(!strcmp(getenv("PS3RECOMP_ASPECT"), "2.0") && !strcmp(getenv("PS3RECOMP_RT_ASPECT"), "auto"));
        unsetenv("PS3RECOMP_RT_ASPECT"); unsetenv("PS3RECOMP_ASPECT");
        remove(p7.c_str());
    }

    remove(path.c_str());
    printf(fails ? "settings_test: %d FAILED\n" : "settings_test: ok\n", fails);
    return fails != 0;
}
