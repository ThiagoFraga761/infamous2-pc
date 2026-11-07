/* The port's screens over the game's window, drawn with Dear ImGui through the
 * toolkit's rsx_ui (libs/video/rsx_ui.h): the in-game menu (F1, or Select+Start
 * held half a second) and the FPS counter. Every change is written to
 * user/settings.ini at once (settings.h); the options marked live apply now,
 * the others at the next start. */
#include "settings.h"
#include "ui.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef PS3RECOMP_UI

#include "imgui.h"
#include "rsx_ui.h"

#include <SDL2/SDL.h>
#ifdef PORT_HAVE_PNG
#include <png.h>
#endif

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif
#include <filesystem>

extern "C" void port_log_restore(void);   /* main.cpp */
extern "C" void rsx_vulkan_backend_set_fullscreen(int on);
extern "C" void rsx_vulkan_backend_set_vsync(int on);
extern "C" void rsx_vulkan_backend_set_stretch(int on);
extern "C" void rsx_vulkan_backend_set_fps_limit(int fps);
extern "C" void rsx_vulkan_backend_set_post_aa(int mode);
extern "C" void rsx_vulkan_backend_set_upscale(int mode);
extern "C" void rsx_vulkan_backend_set_downscale(int mode);
extern "C" void rsx_vulkan_backend_set_sharpen(float s);
extern "C" int rsx_vulkan_backend_fullscreen(void);
extern "C" void ppu_native_set_fov_scale(float k);   /* ppu_native.cpp: CView::SetFov */
extern "C" unsigned ppu_boot_frames_presented(void);
extern "C" volatile int g_savedata_title;   /* libs/system/cellSaveData.c */
extern "C" char g_savedata_load[32];        /* ...and the save to continue */
extern "C" void port_log_drain(void);       /* main.cpp: the log reaches its file */
/* libs/input/cellPad.c (pad_kbm.h): keyboard and mouse as the pad. Called from
 * the menu, which runs on the window's thread like the feed itself. */
extern "C" void cellPad_kbm_set_enabled(int on);
extern "C" void cellPad_kbm_set_sensitivity(float s);
extern "C" void cellPad_kbm_set_invert_y(int on);
extern "C" void cellPad_kbm_set_deadzone(float d);
extern "C" const char* cellPad_kbm_binding(int i, char* key, int keycap);

namespace {

struct Lang { const char* pack; const char* name; bool voice; };
const Lang k_langs[] = {
    { "brazilian",  "Português (Brasil)",          true },
    { "english",    "Inglês",                       true },
    { "latino",     "Espanhol (América Latina)",    true },
    { "spanish",    "Espanhol (Espanha)",           true },
    { "french",     "Francês",                      true },
    { "german",     "Alemão",                       true },
    { "italian",    "Italiano",                     true },
    { "portuguese", "Português (Portugal)",         true },
    { "russian",    "Russo",                        true },
    { "polish",     "Polonês",                      true },
    { "dutch",      "Holandês",                     true },
    { "turkish",    "Turco",                        true },
    { "japanese",   "Japonês",                      true },
    { "british",    "Inglês (Reino Unido)",         false },
    { "korean",     "Coreano (experimental)",       false },
    { "chinese",    "Chinês (experimental)",        false },
};
const char* const k_scales[] = { "720p (1x)", "1440p (2x)", "4K (3x)", "5K (4x, supersampling)" };
const char* const k_aa[] = { "Desligado", "FXAA", "SMAA 1x" };
const char* const k_up[] = { "Bilinear", "FSR 1" };
const char* const k_down[] = { "Bilinear", "Média por área" };
const int k_fps_limits[] = { 0, 30, 40, 60, 80, 120, 240, -1 };
const char* const k_fps_names[] = { "Sem limite", "30", "40", "60", "80", "120", "240", "Taxa do monitor" };
const int k_fps_n = (int)(sizeof k_fps_limits / sizeof k_fps_limits[0]);

port_settings g_boot;          /* as the game started: what a restart would change */
int g_argc; char** g_argv;
std::atomic<int> g_go{1};      /* the main thread may boot the guest */
bool g_want_start = false;     /* the start screen was asked for */
std::atomic<bool> g_start{false};  /* ...and is showing */
double g_fps = 0.0, g_fps_t = 0.0;
unsigned g_fps_frames = 0;
bool g_styled = false;
bool g_backup_failed = false;  /* "Play" refused: the saves could not be backed up */

void style()
{
    ImGuiStyle& s = ImGui::GetStyle();
    ImGui::StyleColorsDark(&s);
    s.WindowRounding = 10.0f; s.FrameRounding = 6.0f; s.TabRounding = 6.0f;
    s.GrabRounding = 6.0f; s.PopupRounding = 6.0f;
    s.WindowPadding = ImVec2(22, 18); s.FramePadding = ImVec2(12, 7);
    s.ItemSpacing = ImVec2(12, 12); s.WindowBorderSize = 0.0f;
    ImVec4* c = s.Colors;
    const ImVec4 accent(0.27f, 0.78f, 1.00f, 1.00f);                 /* #46c8ff */
    c[ImGuiCol_WindowBg]        = ImVec4(0.04f, 0.06f, 0.10f, 0.94f);
    c[ImGuiCol_PopupBg]         = ImVec4(0.06f, 0.08f, 0.13f, 0.98f);
    c[ImGuiCol_FrameBg]         = ImVec4(0.11f, 0.14f, 0.20f, 1.00f);
    c[ImGuiCol_FrameBgHovered]  = ImVec4(0.15f, 0.20f, 0.29f, 1.00f);
    c[ImGuiCol_FrameBgActive]   = ImVec4(0.17f, 0.24f, 0.35f, 1.00f);
    c[ImGuiCol_Tab]             = ImVec4(0.10f, 0.13f, 0.19f, 1.00f);
    c[ImGuiCol_TabHovered]      = ImVec4(0.18f, 0.40f, 0.55f, 1.00f);
    c[ImGuiCol_TabSelected]     = ImVec4(0.13f, 0.42f, 0.60f, 1.00f);
    c[ImGuiCol_Button]          = ImVec4(0.13f, 0.18f, 0.27f, 1.00f);
    c[ImGuiCol_ButtonHovered]   = ImVec4(0.18f, 0.40f, 0.55f, 1.00f);
    c[ImGuiCol_ButtonActive]    = ImVec4(0.20f, 0.52f, 0.72f, 1.00f);
    c[ImGuiCol_Header]          = ImVec4(0.13f, 0.18f, 0.27f, 1.00f);
    c[ImGuiCol_HeaderHovered]   = ImVec4(0.18f, 0.40f, 0.55f, 1.00f);
    c[ImGuiCol_HeaderActive]    = ImVec4(0.20f, 0.52f, 0.72f, 1.00f);
    c[ImGuiCol_CheckMark]       = accent;
    c[ImGuiCol_SliderGrab]      = accent;
    c[ImGuiCol_NavHighlight]    = accent;
    c[ImGuiCol_Separator]       = ImVec4(0.20f, 0.25f, 0.33f, 1.00f);
    c[ImGuiCol_Text]            = ImVec4(0.90f, 0.93f, 0.97f, 1.00f);
    c[ImGuiCol_TextDisabled]    = ImVec4(0.55f, 0.60f, 0.68f, 1.00f);
}

void save()
{
    if (port_settings_save(port_settings_get(), port_settings_path()))
        fprintf(stderr, "[ui] could not write %s\n", port_settings_path());
}

int lang_index(const char* pack)
{
    for (int i = 0; i < (int)(sizeof k_langs / sizeof k_langs[0]); i++)
        if (!strcmp(k_langs[i].pack, pack)) return i;
    return 0;
}

/* A combo over the language list; only voice packs when `voice`. */
bool lang_combo(const char* label, char* pack, size_t cap, bool voice)
{
    const Lang& cur = k_langs[lang_index(pack)];
    bool changed = false;
    if (ImGui::BeginCombo(label, cur.name)) {
        for (const Lang& l : k_langs) {
            if (voice && !l.voice) continue;
            const bool sel = !strcmp(l.pack, pack);
            if (ImGui::Selectable(l.name, sel) && !sel) {
                snprintf(pack, cap, "%s", l.pack);
                changed = true;
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

void hint(const char* t)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::TextWrapped("%s", t);
    ImGui::PopStyleColor();
}

/* A row an environment variable decides (settings.h): shown, not editable. */
bool pinned(unsigned bit) { return (port_settings_pinned() & bit) != 0; }
void pin_begin(unsigned bit) { ImGui::BeginDisabled(pinned(bit)); }
void pin_end(unsigned bit)
{
    ImGui::EndDisabled();
    if (pinned(bit) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Definido por uma variável de ambiente (play.sh ou o terminal)");
}

const char* saves_dir()
{
    const char* e = getenv("PS3_SAVEDATA_ROOT");
    return (e && *e) ? e : "gamedata/dev_hdd0/home/00000001/savedata";
}

#ifdef _WIN32
/* One argument onto a CreateProcess command line, quoted the way the CRT's
 * argv parser takes it apart again. */
void win_quote_arg(std::string& cmd, const char* a)
{
    if (!cmd.empty()) cmd += ' ';
    if (*a && !strpbrk(a, " \t\n\v\"")) { cmd += a; return; }
    cmd += '"';
    for (const char* p = a;; p++) {
        size_t bs = 0;
        while (*p == '\\') { p++; bs++; }
        if (!*p) { cmd.append(bs * 2, '\\'); break; }
        cmd.append(*p == '"' ? bs * 2 + 1 : bs, '\\');
        cmd += *p;
    }
    cmd += '"';
}

/* What execve does on Linux: the same binary, the same arguments, the new
 * environment. Windows has no exec, so a new process and this one exits.
 * Returns only on failure. */
void win_restart(char** argv, char** env)
{
    char self[4096];
    const DWORD n = GetModuleFileNameA(NULL, self, sizeof self);
    if (!n || n >= sizeof self) return;
    std::string cmd;
    for (char** a = argv; *a; a++) win_quote_arg(cmd, *a);
    std::string block;
    for (char** e = env; *e; e++) { block += *e; block += '\0'; }
    block += '\0';
    STARTUPINFOA si = {};
    si.cb = sizeof si;
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessA(self, &cmd[0], NULL, NULL, TRUE, 0, &block[0], NULL, &si, &pi)) return;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    _Exit(0);
}
#endif

void open_folder(const char* path)
{
#ifdef _WIN32
    char full[MAX_PATH];
    if (!GetFullPathNameA(path, sizeof full, full, NULL)) snprintf(full, sizeof full, "%s", path);
    for (char* p = full; *p; p++) if (*p == '/') *p = '\\';
    if ((INT_PTR)ShellExecuteA(NULL, "open", full, NULL, NULL, SW_SHOWNORMAL) <= 32)
        fprintf(stderr, "[ui] could not open %s\n", path);
#else
    /* posix_spawn, not system(): a fork of this process copies its page
     * tables and stops every thread for seconds. */
    char* argv[] = { (char*)"xdg-open", (char*)path, NULL };
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_addopen(&fa, 1, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addopen(&fa, 2, "/dev/null", O_WRONLY, 0);
    pid_t pid;
    if (posix_spawnp(&pid, "xdg-open", &fa, NULL, argv, environ) != 0)
        fprintf(stderr, "[ui] could not open %s\n", path);
    else
        std::thread([pid] { int st; waitpid(pid, &st, 0); }).detach();
    posix_spawn_file_actions_destroy(&fa);
#endif
}

std::string g_last_backup;

bool backup_saves()
{
    struct stat st;
    if (stat(saves_dir(), &st) != 0) { g_last_backup = "Nenhum save para copiar"; return true; }
    char ts[32];
    const time_t t = time(nullptr);
    strftime(ts, sizeof ts, "%Y%m%d-%H%M%S", localtime(&t));
    const char* u = getenv("PS3_USER_DIR");
    const std::string dst = std::string(u && *u ? u : "user") + "/backups/" + ts;
    /* In process: system() would fork, and a fork stops the game for seconds. */
    std::error_code ec;
    std::filesystem::create_directories(dst, ec);
    if (!ec) std::filesystem::copy(saves_dir(), dst, std::filesystem::copy_options::recursive, ec);
    const bool ok = !ec;
    g_last_backup = ok ? "Backup em " + dst : "O backup dos saves falhou";
    return ok;
}

void page_video(port_settings& s)
{
    int sc = s.scale - 1;
    pin_begin(PORT_PIN_SCALE);
    if (ImGui::Combo("Resolução", &sc, k_scales, 4)) { s.scale = sc + 1; save(); }
    pin_end(PORT_PIN_SCALE);
    /* The present chain: these four take effect at the next frame. */
    pin_begin(PORT_PIN_POST_AA);
    if (ImGui::Combo("Anti-aliasing", &s.post_aa, k_aa, 3)) { rsx_vulkan_backend_set_post_aa(s.post_aa); save(); }
    pin_end(PORT_PIN_POST_AA);
    pin_begin(PORT_PIN_UPSCALE);
    if (ImGui::Combo("Ampliar para a janela", &s.upscale, k_up, 2)) { rsx_vulkan_backend_set_upscale(s.upscale); save(); }
    pin_end(PORT_PIN_UPSCALE);
    pin_begin(PORT_PIN_DOWNSCALE);
    if (ImGui::Combo("Reduzir para a janela", &s.downscale, k_down, 2)) {
        rsx_vulkan_backend_set_downscale(s.downscale); save();
    }
    pin_end(PORT_PIN_DOWNSCALE);
    pin_begin(PORT_PIN_SHARPEN);
    bool autos = s.sharpen < 0;
    if (ImGui::Checkbox("Nitidez automática", &autos)) {
        s.sharpen = autos ? -1 : 0;
        rsx_vulkan_backend_set_sharpen(autos ? -1.0f : 0.0f); save();
    }
    if (!autos) {
        ImGui::SameLine();
        float f = s.sharpen / 1000.0f;
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.0f);
        if (ImGui::SliderFloat("Nitidez", &f, 0.0f, 1.0f, "%.2f")) {
            s.sharpen = (int)(f * 1000.0f + 0.5f);
            rsx_vulkan_backend_set_sharpen(f);
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) save();
    }
    pin_end(PORT_PIN_SHARPEN);
    /* The window as it is: F11 changes it without the menu. */
    bool b = rsx_vulkan_backend_fullscreen();
    if (ImGui::Checkbox("Tela cheia  (F11)", &b)) { s.fullscreen = b; rsx_vulkan_backend_set_fullscreen(b); save(); }
    b = s.vsync;
    if (ImGui::Checkbox("VSync", &b)) { s.vsync = b; rsx_vulkan_backend_set_vsync(b); save(); }
    b = s.stretch;
    if (ImGui::Checkbox("Esticar a imagem (sem barras pretas)", &b)) {
        s.stretch = b; rsx_vulkan_backend_set_stretch(b); save();
    }
    /* Live: each camera takes the new scale on its next SetFov, which the game
     * calls every frame. Saved once the slider is let go, not on every step. */
    int fov = s.fov;
    pin_begin(PORT_PIN_FOV);
    if (ImGui::SliderInt("Campo de visão", &fov, 0, 30, fov ? "+%d%%" : "Do jogo", ImGuiSliderFlags_AlwaysClamp)) {
        s.fov = fov; ppu_native_set_fov_scale(1.0f + fov / 100.0f);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) save();
    pin_end(PORT_PIN_FOV);
    /* From the next start: the targets' size and the camera are set once. */
    b = s.ultrawide;
    pin_begin(PORT_PIN_ULTRAWIDE);
    if (ImGui::Checkbox("Ultrawide (imagem no formato do monitor)", &b)) { s.ultrawide = b; save(); }
    pin_end(PORT_PIN_ULTRAWIDE);
    if (const char* a = getenv("PS3RECOMP_RT_ASPECT")) {
        ImGui::SameLine();
        ImGui::TextDisabled("agora: %s", a);
    }
    const int nd = SDL_GetNumVideoDisplays();
    if (nd > 1 && !pinned(PORT_PIN_DISPLAY)) {
        const char* cur = SDL_GetDisplayName(s.display < nd ? s.display : 0);
        if (ImGui::BeginCombo("Monitor", cur ? cur : "?")) {
            for (int i = 0; i < nd; i++) {
                const char* n = SDL_GetDisplayName(i);
                char label[160]; snprintf(label, sizeof label, "%d: %s", i + 1, n ? n : "?");
                if (ImGui::Selectable(label, i == s.display)) { s.display = i; save(); }
            }
            ImGui::EndCombo();
        }
    }
    hint("Resolução e monitor valem a partir da próxima vez que o jogo abrir; o resto, na hora. "
         "FSR 1 amplia quando a imagem é menor que a janela (720p numa tela 1440p); a média por "
         "área reduz quando é maior (4K ou 5K numa tela 1440p). O anti-aliasing também suaviza o HUD do jogo. "
         "Campo de visão: mais mundo na tela também é mais trabalho para a CPU. "
         "Ultrawide (vale na próxima vez que o jogo abrir): o mundo ocupa o monitor inteiro, "
         "21:9 ou 32:9, e o HUD, os menus e os vídeos ficam no meio, em 16:9. Também pesa na CPU. "
         "Em monitor 16:9 não muda nada.");
}

void page_language(port_settings& s)
{
    pin_begin(PORT_PIN_VOICE);
    if (lang_combo("Dublagem", s.voice, sizeof s.voice, true)) save();
    pin_end(PORT_PIN_VOICE);
    pin_begin(PORT_PIN_TEXT);
    if (lang_combo("Legendas e textos", s.text, sizeof s.text, false)) save();
    pin_end(PORT_PIN_TEXT);
    hint("Dá para misturar, por exemplo vozes em inglês e legendas em português. "
         "Ligar ou desligar as legendas fica no menu de opções do próprio jogo. "
         "O idioma vale a partir da próxima vez que o jogo abrir.");
}

void page_performance(port_settings& s)
{
    ImGui::TextUnformatted("Predefinição");
    const char* names[] = { "Desempenho (720p)", "Equilibrado (1440p)", "Qualidade (4K)" };
    pin_begin(PORT_PIN_SCALE | PORT_PIN_RECORD_THREAD);
    for (int i = 0; i < 3; i++) {
        if (i) ImGui::SameLine();
        if (ImGui::Button(names[i])) { s.scale = i + 1; s.record_thread = 1; save(); }
    }
    pin_end(PORT_PIN_SCALE | PORT_PIN_RECORD_THREAD);
    int li = 0;
    for (int i = 0; i < k_fps_n; i++) if (k_fps_limits[i] == s.fps_limit) li = i;
    if (s.fps_limit > 0 && k_fps_limits[li] != s.fps_limit) li = k_fps_n - 1;
    pin_begin(PORT_PIN_FPS_LIMIT);
    if (ImGui::Combo("Limite de FPS", &li, k_fps_names, k_fps_n)) {
        int v = k_fps_limits[li];
        if (v < 0) {
            SDL_DisplayMode dm;
            v = SDL_GetCurrentDisplayMode(rsx_ui_display_index(), &dm) == 0 && dm.refresh_rate > 0
                ? dm.refresh_rate : 60;
        }
        s.fps_limit = v; rsx_vulkan_backend_set_fps_limit(v); save();
    }
    pin_end(PORT_PIN_FPS_LIMIT);
    bool b = s.fps_counter;
    if (ImGui::Checkbox("Contador de FPS na tela", &b)) { s.fps_counter = b; rsx_ui_set_visible(b); save(); }
    b = s.record_thread;
    pin_begin(PORT_PIN_RECORD_THREAD);
    if (ImGui::Checkbox("Thread de gravação Vulkan (+12% nas áreas pesadas)", &b)) { s.record_thread = b; save(); }
    pin_end(PORT_PIN_RECORD_THREAD);
    hint("A thread de gravação vale a partir da próxima vez que o jogo abrir.");
}

void page_mods(port_settings& s)
{
    ImGui::TextUnformatted("Além do PS3");
    bool b = s.fullres_particles;
    pin_begin(PORT_PIN_PARTICLES);
    if (ImGui::Checkbox("Partículas em resolução cheia", &b)) { s.fullres_particles = b; save(); }
    pin_end(PORT_PIN_PARTICLES);
    hint("Fogo, faíscas, fumaça e raios: o jogo os desenha em 512x288 e estica; "
         "ligado, saem na resolução do resto da imagem.");
    static const char* const k_aniso_names[] = { "Do jogo", "2x", "4x", "8x", "16x" };
    static const int k_aniso[] = { 0, 2, 4, 8, 16 };
    int ai = 0;
    for (int i = 0; i < 5; i++) if (k_aniso[i] == s.aniso) ai = i;
    pin_begin(PORT_PIN_ANISO);
    if (ImGui::Combo("Filtro anisotrópico", &ai, k_aniso_names, 5)) { s.aniso = k_aniso[ai]; save(); }
    pin_end(PORT_PIN_ANISO);
    hint("Chão e paredes nítidos à distância e em ângulo. O PS3 quase não usava.");

    ImGui::Separator();
    ImGui::TextUnformatted("Mods instalados");
    static port_mod mods[64];
    static int nmods = -1;
    if (nmods < 0 || ImGui::Button("Procurar de novo")) nmods = port_mods_scan(mods, 64);
    if (!nmods) {
        hint("Nenhum mod na pasta mods/. Cada mod é uma pasta com files/ espelhando o USRDIR do "
             "jogo e, se quiser, um mod.ini (name, author, version, description). "
             "Detalhes em docs/MODS.md.");
    }
    for (int i = 0; i < nmods; i++) {
        bool on = port_mod_enabled(&s, mods[i].dir);
        char label[200];
        snprintf(label, sizeof label, "%s%s%s##%s", mods[i].name, mods[i].version[0] ? "  v" : "",
                 mods[i].version, mods[i].dir);
        if (ImGui::Checkbox(label, &on)) { port_mod_set(&s, mods[i].dir, on); save(); }
        if (mods[i].description[0] || mods[i].author[0]) {
            char d[400];
            snprintf(d, sizeof d, "%s%s%s", mods[i].description, mods[i].author[0] ? "  -- " : "", mods[i].author);
            hint(d);
        }
    }
    hint("Os mods ligados por último têm prioridade quando dois trocam o mesmo arquivo. "
         "As opções desta aba valem a partir da próxima vez que o jogo abrir.");
}

void page_controller(port_settings& s)
{
    SDL_GameController* gc = nullptr;
    for (int i = 0; i < SDL_NumJoysticks() && !gc; i++)
        if (SDL_IsGameController(i)) gc = SDL_GameControllerFromInstanceID(SDL_JoystickGetDeviceInstanceID(i));
    const char* name = gc ? SDL_GameControllerName(gc) : nullptr;
    ImGui::Text("Controle: %s", name ? name : "nenhum encontrado (conecte antes de abrir o jogo)");
    if (gc) {
        static const struct { SDL_GameControllerButton b; const char* n; } k[] = {
            { SDL_CONTROLLER_BUTTON_A, "Xis/A" }, { SDL_CONTROLLER_BUTTON_B, "Bola/B" },
            { SDL_CONTROLLER_BUTTON_X, "Quadrado/X" }, { SDL_CONTROLLER_BUTTON_Y, "Triângulo/Y" },
            { SDL_CONTROLLER_BUTTON_LEFTSHOULDER, "L1" }, { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, "R1" },
            { SDL_CONTROLLER_BUTTON_BACK, "Select" }, { SDL_CONTROLLER_BUTTON_START, "Start" },
            { SDL_CONTROLLER_BUTTON_LEFTSTICK, "L3" }, { SDL_CONTROLLER_BUTTON_RIGHTSTICK, "R3" },
        };
        std::string held;
        for (const auto& e : k)
            if (SDL_GameControllerGetButton(gc, e.b)) { held += held.empty() ? "" : "  "; held += e.n; }
        ImGui::Text("Apertados: %s", held.empty() ? "nenhum" : held.c_str());
    }
    ImGui::Separator();
    if (ImGui::Button("Abrir pasta dos saves")) open_folder(saves_dir());
    ImGui::SameLine();
    if (ImGui::Button("Fazer backup dos saves")) backup_saves();
    ImGui::SameLine();
    if (ImGui::Button("Abrir pasta dos logs")) open_folder("work/runs");
    if (!g_last_backup.empty()) hint(g_last_backup.c_str());
    hint("Continuar, jogo novo e carregar ficam no menu da tela inicial do port; dentro do jogo, "
         "em Start > Dados salvos.");
    ImGui::Separator();
    bool b = s.skip_launcher;
    if (ImGui::Checkbox("Pular a tela de opções ao abrir o jogo (F1 abre no jogo)", &b)) { s.skip_launcher = b; save(); }
}

/* A pad control by the name cellPad gives it, in the menu's words. */
const char* kbm_action_label(const char* a)
{
    static const struct { const char* a; const char* t; } k[] = {
        { "CROSS", "Xis" }, { "CIRCLE", "Bola" }, { "SQUARE", "Quadrado" }, { "TRIANGLE", "Triângulo" },
        { "START", "Start" }, { "SELECT", "Select" },
        { "UP", "Direcional: cima" }, { "DOWN", "Direcional: baixo" },
        { "LEFT", "Direcional: esquerda" }, { "RIGHT", "Direcional: direita" },
        { "LS_UP", "Andar: frente" }, { "LS_DOWN", "Andar: trás" },
        { "LS_LEFT", "Andar: esquerda" }, { "LS_RIGHT", "Andar: direita" },
        { "RS_UP", "Câmera: cima" }, { "RS_DOWN", "Câmera: baixo" },
        { "RS_LEFT", "Câmera: esquerda" }, { "RS_RIGHT", "Câmera: direita" },
        { "WALK", "Andar devagar" },
    };
    for (const auto& e : k)
        if (!strcmp(e.a, a)) return e.t;
    return a;   /* L1, R1, L2, R2, L3, R3 */
}

/* A key by the name cellPad gives it, where the menu's words differ. */
const char* kbm_key_label(const char* key)
{
    static const struct { const char* k; const char* t; } m[] = {
        { "MouseLeft", "Mouse: esquerdo" }, { "MouseRight", "Mouse: direito" },
        { "MouseMiddle", "Mouse: meio" }, { "Mouse4", "Mouse: lateral 1" }, { "Mouse5", "Mouse: lateral 2" },
        { "WheelUp", "Roda: para cima" }, { "WheelDown", "Roda: para baixo" },
        { "Space", "Espaço" }, { "LShift", "Shift esquerdo" }, { "LCtrl", "Ctrl esquerdo" },
        { "Up", "Seta para cima" }, { "Down", "Seta para baixo" },
        { "Left", "Seta para a esquerda" }, { "Right", "Seta para a direita" },
    };
    for (const auto& e : m)
        if (!strcmp(e.k, key)) return e.t;
    return key;
}

void page_kbm(port_settings& s)
{
    bool b = s.kbm;
    pin_begin(PORT_PIN_KBM);
    if (ImGui::Checkbox("Jogar com teclado e mouse", &b)) { s.kbm = b; cellPad_kbm_set_enabled(b); save(); }
    pin_end(PORT_PIN_KBM);
    ImGui::BeginDisabled(!s.kbm);
    /* Live; saved once a slider is let go, not on every step. */
    float sens = s.kbm_sens / 100.0f;
    pin_begin(PORT_PIN_KBM_SENS);
    if (ImGui::SliderFloat("Sensibilidade do mouse", &sens, 0.10f, 5.00f, "%.2f",
                           ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) {
        s.kbm_sens = (int)lroundf(sens * 100.0f);
        cellPad_kbm_set_sensitivity(s.kbm_sens / 100.0f);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) save();
    pin_end(PORT_PIN_KBM_SENS);
    b = s.kbm_invert_y;
    pin_begin(PORT_PIN_KBM_INVERT_Y);
    if (ImGui::Checkbox("Inverter o eixo vertical", &b)) { s.kbm_invert_y = b; cellPad_kbm_set_invert_y(b); save(); }
    pin_end(PORT_PIN_KBM_INVERT_Y);
    int dz = s.kbm_deadzone;
    pin_begin(PORT_PIN_KBM_DEADZONE);
    if (ImGui::SliderInt("Início do giro", &dz, 0, 60, "%d%%", ImGuiSliderFlags_AlwaysClamp)) {
        s.kbm_deadzone = dz;
        cellPad_kbm_set_deadzone(dz / 100.0f);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) save();
    pin_end(PORT_PIN_KBM_DEADZONE);
    ImGui::EndDisabled();
    hint("O mouse faz o papel do analógico direito: a câmera gira na velocidade do próprio jogo, "
         "ainda sem mira 1:1. \"Início do giro\" é onde o analógico começa no menor movimento: "
         "suba se o mouse devagar não mexe a câmera, desça se ela dá tranco. O mouse é capturado "
         "no primeiro clique ou tecla dentro do jogo; Alt+Tab e F1 soltam. Funciona junto com o "
         "controle. Para trocar teclas: [keyboard] map em user/settings.ini.");
    /* The layout as cellPad has it (the default plus [keyboard] map). It
     * scrolls inside what is left above the menu's bottom row. */
    const float fs = ImGui::GetFontSize();
    const float h = ImGui::GetContentRegionAvail().y - fs * 3.6f;
    if (h < fs * 3.0f) return;
    if (ImGui::BeginChild("kbm_keys", ImVec2(0, h), ImGuiChildFlags_Borders)) {
        if (ImGui::BeginTable("kbm_map", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            char key[32];
            for (int i = 0;; i++) {
                const char* a = cellPad_kbm_binding(i, key, sizeof key);
                if (!a) break;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(kbm_key_label(key));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(kbm_action_label(a));
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}

/* What changed since the game started and needs a restart. */
std::string restart_note(const port_settings& s)
{
    std::string n;
    auto add = [&](const char* w) { n += n.empty() ? "" : ", "; n += w; };
    if (s.scale != g_boot.scale) add("resolução");
    if (s.display != g_boot.display) add("monitor");
    if (strcmp(s.voice, g_boot.voice) || strcmp(s.text, g_boot.text)) add("idioma");
    if (s.record_thread != g_boot.record_thread) add("thread de gravação");
    if (s.ultrawide != g_boot.ultrawide) add("ultrawide");
    return n;
}

void draw_menu()
{
    port_settings& s = *port_settings_get();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, 150));
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(fs * 38.0f < io.DisplaySize.x * 0.92f ? fs * 38.0f : io.DisplaySize.x * 0.92f,
                                    fs * 26.0f < io.DisplaySize.y * 0.9f ? fs * 26.0f : io.DisplaySize.y * 0.9f),
                             ImGuiCond_Always);
    ImGui::Begin("inFAMOUS 2  ·  Opções", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoSavedSettings);
    if (ImGui::BeginTabBar("pages")) {
        if (ImGui::BeginTabItem("Vídeo")) { page_video(s); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Idioma")) { page_language(s); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Desempenho")) { page_performance(s); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Mods")) { page_mods(s); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Controle e saves")) { page_controller(s); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Teclado e mouse")) { page_kbm(s); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    const std::string note = restart_note(s);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - fs * 3.2f);
    ImGui::Separator();
    if (!note.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.78f, 0.35f, 1.0f));
        ImGui::Text("Reinicie o jogo para aplicar: %s.", note.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();
    }
    /* Escape opens this menu now, so quitting is here, behind a second click:
     * whatever the game has not saved is lost. */
    static bool quit_armed = false;
    if (!rsx_ui_menu_open()) quit_armed = false;
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - fs * (quit_armed ? 22.0f : 14.5f));
    if (ImGui::Button(quit_armed ? "Sair sem salvar? Confirmar" : "Sair do jogo")) {
        if (quit_armed) { port_log_drain(); _Exit(0); }
        quit_armed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Fechar  (F1)")) { quit_armed = false; rsx_ui_set_menu_open(0); }
    ImGui::End();
}

void draw_fps()
{
    const ImGuiIO& io = ImGui::GetIO();
    const char* t = nullptr;
    char b[32];
    snprintf(b, sizeof b, "%.0f fps", g_fps);
    t = b;
    const ImVec2 sz = ImGui::CalcTextSize(t);
    const ImVec2 p(io.DisplaySize.x - sz.x - 18.0f, 12.0f);
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    dl->AddRectFilled(ImVec2(p.x - 8, p.y - 4), ImVec2(p.x + sz.x + 8, p.y + sz.y + 4), IM_COL32(0, 0, 0, 150), 6.0f);
    dl->AddText(p, IM_COL32(120, 220, 255, 255), t);
}

/* The saves, as the "Game" page lists them. */
struct SaveInfo { std::string dir, title, detail, when; time_t mtime; };
std::vector<SaveInfo> g_saves;
bool g_saves_read = false;
int g_choice = -1;             /* -1 continue as the game does, -2 its title screen, else g_saves[i] */

/* One string from a PARAM.SFO: the console's binary one (little-endian
 * header and index, UTF-8 data), or the "KEY=value" lines cellSaveData
 * writes, where a line with no '=' continues the value above it. */
std::string sfo_field(const std::vector<unsigned char>& d, const char* key)
{
    if (d.size() < 4 || memcmp(d.data(), "\0PSF", 4)) {
        const std::string t(d.begin(), d.end()), k = std::string(key) + "=";
        std::string v;
        bool in = false;
        for (size_t a = 0; a < t.size();) {
            size_t b = t.find('\n', a);
            if (b == std::string::npos) b = t.size();
            const std::string line = t.substr(a, b - a);
            if (!line.compare(0, k.size(), k)) { v = line.substr(k.size()); in = true; }
            else if (in && line.find('=') == std::string::npos) v += "\n" + line;
            else if (in) break;
            a = b + 1;
        }
        return v;
    }
    auto u32 = [&](size_t o) { return o + 4 <= d.size() ? (unsigned)d[o] | (unsigned)d[o + 1] << 8 |
                                                         (unsigned)d[o + 2] << 16 | (unsigned)d[o + 3] << 24 : 0u; };
    auto u16 = [&](size_t o) { return o + 2 <= d.size() ? (unsigned)d[o] | (unsigned)d[o + 1] << 8 : 0u; };
    if (d.size() < 20 || memcmp(d.data(), "\0PSF", 4)) return {};
    const unsigned keys = u32(8), data = u32(12), n = u32(16);
    for (unsigned i = 0; i < n && 20 + i * 16 + 16 <= d.size(); i++) {
        const size_t e = 20 + (size_t)i * 16;
        const size_t ko = keys + u16(e), dl = u32(e + 4), dof = data + u32(e + 12);
        if (ko >= d.size() || dof + dl > d.size()) continue;
        if (strcmp((const char*)d.data() + ko, key)) continue;
        std::string v((const char*)d.data() + dof, dl);
        v.resize(strnlen(v.c_str(), v.size()));
        return v;
    }
    return {};
}

void read_saves()
{
    g_saves.clear();
    DIR* dir = opendir(saves_dir());
    if (!dir) return;
    while (dirent* e = readdir(dir)) {
        if (e->d_name[0] == '.') continue;
        const std::string sfo = std::string(saves_dir()) + "/" + e->d_name + "/PARAM.SFO";
        struct stat st;
        if (strlen(e->d_name) >= 32 || stat(sfo.c_str(), &st) != 0) continue;
        FILE* f = fopen(sfo.c_str(), "rb");
        if (!f) continue;
        std::vector<unsigned char> d(st.st_size > 0 && st.st_size < 65536 ? (size_t)st.st_size : 0);
        const size_t got = d.empty() ? 0 : fread(d.data(), 1, d.size(), f);
        fclose(f);
        d.resize(got);
        SaveInfo si;
        si.dir = e->d_name;
        si.title = sfo_field(d, "SUB_TITLE");
        si.detail = sfo_field(d, "DETAIL");
        si.detail = si.detail.substr(0, si.detail.find('\n'));
        if (si.title.empty()) si.title = si.dir;
        si.mtime = st.st_mtime;
        char when[32];
        strftime(when, sizeof when, "%d/%m %H:%M", localtime(&si.mtime));
        si.when = when;
        g_saves.push_back(si);
    }
    closedir(dir);
    std::sort(g_saves.begin(), g_saves.end(), [](const SaveInfo& a, const SaveInfo& b) { return a.mtime > b.mtime; });
}

/* "Play": boot, or start the process again when an option that only takes
 * effect at start changed here, starting where the menu says (g_choice). */
void play()
{
    port_settings& s = *port_settings_get();
    save();
    const bool title = g_choice == -2;
    const bool other = g_choice >= 0 && g_choice < (int)g_saves.size();
    /* No backup, no changing where it starts: the next autosave would write
     * over the newest save with nothing to go back to. The screen stays. */
    if ((title || other) && !backup_saves()) {
        fprintf(stderr, "[ui] the saves could not be backed up: starting where the game would\n");
        g_backup_failed = true;
        return;
    }
    static std::string extra;
    if (title) {
        fprintf(stderr, "[ui] the game's title screen (%s)\n", g_last_backup.c_str());
        extra = "PS3_SAVEDATA_TITLE=1";
    } else if (other) {
        fprintf(stderr, "[ui] continuing %s (%s)\n", g_saves[g_choice].dir.c_str(), g_last_backup.c_str());
        extra = "PS3_SAVEDATA_LOAD=" + g_saves[g_choice].dir;
    }
    if (!restart_note(s).empty() && g_argv) {
        fprintf(stderr, "[ui] starting again with the new settings\n");
        fflush(stderr); fflush(stdout);
        if (char** env = port_settings_restart_env(extra.empty() ? nullptr : extra.c_str())) {
            /* The binary's own path, not /proc/self/exe: the process keeps
             * its name (MyGameRecomp), which play.sh and the scripts look for. */
#ifdef _WIN32
            win_restart(g_argv, env);
#else
            port_log_restore();
            char self[4096];
            const ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
            if (n > 0) { self[n] = 0; execve(self, g_argv, env); }
            execve("/proc/self/exe", g_argv, env);
#endif
            free(env);
        }
        fprintf(stderr, "[ui] could not start again: the old settings stay for this run\n");
    }
    if (title) g_savedata_title = 1;
    if (other) snprintf(g_savedata_load, sizeof g_savedata_load, "%s", g_saves[g_choice].dir.c_str());
    g_start = false;
    rsx_ui_set_modal(0);
    rsx_ui_set_menu_open(0);
    rsx_ui_set_visible(s.fps_counter);
    g_go = 1;
}

/* The start screen: a game's main menu. inFAMOUS 2 has none of its own --
 * with a save it continues it, without one it shows its new-game screen -- so
 * this is that menu, over the disc's own picture (PIC1.PNG). Continue, a new
 * game (the game's own new-game screen), load a save, the port's options,
 * quit. A pad moves through it as a keyboard or a mouse does. */
enum MenuPage { MENU_MAIN, MENU_LOAD, MENU_NEW, MENU_OPTIONS };
MenuPage g_page = MENU_MAIN;
bool g_page_focus = true;      /* focus the page's first item on its first frame */

const ImU32 k_accent = IM_COL32(70, 200, 255, 255);

/* One line of the menu: big text, an optional line under it, a bar beside
 * the one under the pointer or the pad. */
bool menu_item(const char* id, const char* label, const char* sub, float scale = 1.0f)
{
    const float fs = ImGui::GetFontSize();
    ImFont* tf = (ImFont*)rsx_ui_title_font();
    const float big = fs * 1.75f * scale;
    const float h = big * 1.25f + (sub ? fs * 1.35f : 0.0f);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    if (g_page_focus) { ImGui::SetKeyboardFocusHere(); g_page_focus = false; }
    const bool pressed = ImGui::Selectable(id, false, 0, ImVec2(ImGui::GetContentRegionAvail().x, h));
    const bool hot = ImGui::IsItemHovered() || ImGui::IsItemFocused();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (hot) dl->AddRectFilled(p, ImVec2(p.x + fs * 0.22f, p.y + h - fs * 0.25f), k_accent);
    dl->AddText(tf, big, ImVec2(p.x + fs * 0.85f, p.y), hot ? IM_COL32(255, 255, 255, 255)
                                                            : IM_COL32(190, 196, 206, 210), label);
    if (sub)
        dl->AddText(ImVec2(p.x + fs * 0.9f, p.y + big * 1.12f), hot ? IM_COL32(150, 215, 255, 255)
                                                                    : IM_COL32(150, 158, 170, 190), sub);
    return pressed;
}

/* PIC1.PNG filling the window, darkened on the left where the menu is. */
void draw_backdrop(const ImVec2& d)
{
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    int iw = 0, ih = 0;
    if (void* tex = rsx_ui_image(&iw, &ih)) {
        const float da = d.x / d.y, ia = (float)iw / (float)ih;
        ImVec2 uv0(0, 0), uv1(1, 1);
        if (da > ia) { const float v = ia / da; uv0.y = (1 - v) * 0.5f; uv1.y = 1 - uv0.y; }
        else         { const float u = da / ia; uv0.x = (1 - u) * 0.5f; uv1.x = 1 - uv0.x; }
        bg->AddImage((ImTextureID)(uintptr_t)tex, ImVec2(0, 0), d, uv0, uv1);
    } else {
        bg->AddRectFilledMultiColor(ImVec2(0, 0), d, IM_COL32(14, 22, 38, 255), IM_COL32(8, 12, 22, 255),
                                    IM_COL32(2, 3, 6, 255), IM_COL32(4, 6, 12, 255));
    }
    const ImU32 dark = IM_COL32(0, 0, 0, 225), clear = IM_COL32(0, 0, 0, 0);
    bg->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(d.x * 0.65f, d.y), dark, clear, clear, dark);
    bg->AddRectFilled(ImVec2(0, 0), d, IM_COL32(0, 0, 0, g_page == MENU_OPTIONS ? 120 : 40));
}

void go_page(MenuPage p) { g_page = p; g_page_focus = true; }

/* A sub-page's name, in the menu's type. */
void page_title(const char* t)
{
    const float fs = ImGui::GetFontSize();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddText((ImFont*)rsx_ui_title_font(), fs * 1.3f, ImVec2(p.x + fs * 0.85f, p.y),
                                        k_accent, t);
    ImGui::Dummy(ImVec2(0, fs * 2.0f));
}

void draw_start()
{
    port_settings& s = *port_settings_get();
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 d = io.DisplaySize;
    const float fs = ImGui::GetFontSize();
    if (!g_saves_read) { read_saves(); g_saves_read = true; }
    draw_backdrop(d);

    /* the title, top left */
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    ImFont* tf = (ImFont*)rsx_ui_title_font();
    const ImVec2 tp(d.x * 0.07f, d.y * 0.10f);
    const float th = fs * 3.4f;
    bg->AddText(tf, th, tp, IM_COL32(255, 255, 255, 255), "inFAMOUS 2");
    bg->AddRectFilled(ImVec2(tp.x + 4, tp.y + th * 1.06f), ImVec2(tp.x + 4 + fs * 5.0f, tp.y + th * 1.06f + 5), k_accent);

    /* Circle, Escape: back to the main page. */
    if (g_page != MENU_MAIN &&
        (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)))
        go_page(MENU_MAIN);

    const float top = tp.y + th * 1.06f + fs * 2.4f;
    const float width = g_page == MENU_OPTIONS ? (fs * 38.0f < d.x * 0.86f ? fs * 38.0f : d.x * 0.86f)
                                               : (fs * 34.0f < d.x * 0.62f ? fs * 34.0f : d.x * 0.62f);
    const float height = d.y - top - fs * 3.2f;
    ImGui::SetNextWindowPos(ImVec2(tp.x - fs * 0.9f, top), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    if (g_page != MENU_OPTIONS) ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_NavHighlight, ImVec4(0, 0, 0, 0));
    ImGui::Begin("##start", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoScrollbar);
    bool go = false;
    switch (g_page) {
    case MENU_MAIN: {
        if (!g_saves.empty()) {
            const SaveInfo& n = g_saves[0];
            const std::string sub = n.title + "   ·   " + n.detail + "   ·   " + n.when;
            if (menu_item("##continue", "CONTINUAR", sub.c_str())) {
                /* The newest save. The game would pick its autosave itself;
                 * naming it only when it is another one keeps the usual
                 * start free of a backup. */
                const size_t k = n.dir.size();
                g_choice = (k >= 2 && n.dir.compare(k - 2, 2, "_0") == 0) ? -1 : 0;
                go = true;
            }
        }
        if (menu_item("##new", "NOVO JOGO", nullptr)) {
            if (g_saves.empty()) { g_choice = -1; go = true; }   /* the game's new-game screen comes anyway */
            else go_page(MENU_NEW);
        }
        if (!g_saves.empty() && menu_item("##load", "CARREGAR JOGO", nullptr)) go_page(MENU_LOAD);
        if (menu_item("##options", "OPÇÕES", nullptr)) go_page(MENU_OPTIONS);
        if (menu_item("##quit", "SAIR", nullptr)) {
            port_log_drain();
            _Exit(0);
        }
        break; }
    case MENU_LOAD: {
        page_title("CARREGAR JOGO");
        for (int i = 0; i < (int)g_saves.size(); i++) {
            const SaveInfo& si = g_saves[i];
            const std::string sub = si.detail + "   ·   " + si.when;
            char id[32]; snprintf(id, sizeof id, "##save%d", i);
            if (menu_item(id, si.title.c_str(), sub.c_str(), 0.62f)) { g_choice = i; go = true; }
        }
        ImGui::Spacing();
        hint("O jogo grava o autosave por cima do atual logo depois de carregar, como no PS3. "
             "Antes, os saves são copiados para user/backups.");
        break; }
    case MENU_NEW: {
        page_title("NOVO JOGO");
        hint("Abre a tela de jogo novo do próprio inFAMOUS 2. Quando o jogo novo salvar, o autosave atual "
             "é substituído, como no PS3; antes, todos os saves são copiados para user/backups.");
        ImGui::Spacing();
        if (menu_item("##newyes", "COMEÇAR", nullptr, 0.8f)) { g_choice = -2; go = true; }
        if (menu_item("##newno", "VOLTAR", nullptr, 0.8f)) go_page(MENU_MAIN);
        break; }
    case MENU_OPTIONS: {
        ImGui::PopStyleColor(4);   /* the options' own widgets keep their colours */
        if (ImGui::BeginTabBar("pages")) {
            if (ImGui::BeginTabItem("Vídeo")) { page_video(s); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Idioma")) { page_language(s); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Desempenho")) { page_performance(s); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Mods")) { page_mods(s); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Controle e saves")) { page_controller(s); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Teclado e mouse")) { page_kbm(s); ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
        const std::string note = restart_note(s);
        ImGui::SetCursorPosY(height - fs * 3.0f);
        ImGui::Separator();
        if (g_page_focus) { ImGui::SetKeyboardFocusHere(); g_page_focus = false; }
        if (ImGui::Button("VOLTAR", ImVec2(fs * 8.0f, fs * 1.8f))) go_page(MENU_MAIN);
        ImGui::SameLine();
        hint(note.empty() ? "As opções ficam gravadas. F1 (ou Select+Start segurados) abre esta tela no jogo."
                          : ("Ao entrar no jogo ele reabre para aplicar: " + note + ".").c_str());
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));   /* rebalance the pop above */
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_NavHighlight, ImVec4(0, 0, 0, 0));
        break; }
    }
    if (g_backup_failed) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.55f, 0.45f, 1.0f));
        ImGui::TextWrapped("Não deu para copiar os saves para user/backups, então o jogo não começou. "
                           "Veja o log.");
        ImGui::PopStyleColor();
    }
    ImGui::End();
    ImGui::PopStyleColor(4);

    /* the controls, bottom left */
    const char* keys = g_page == MENU_MAIN ? "X / Enter  selecionar" : "X / Enter  selecionar      O / Esc  voltar";
    bg->AddText(ImVec2(tp.x, d.y - fs * 2.2f), IM_COL32(200, 205, 214, 200), keys);
    if (go) play();
}

void draw(void*)
{
    if (!g_styled) { style(); g_styled = true; }
    /* Frames presented over half a second. */
    g_fps_t += rsx_ui_frame_dt();
    if (g_fps_t >= 0.5) {
        const unsigned f = ppu_boot_frames_presented();
        g_fps = (double)(f - g_fps_frames) / g_fps_t;
        g_fps_frames = f; g_fps_t = 0.0;
    }
    if (g_start) { draw_start(); return; }
    if (port_settings_get()->fps_counter) draw_fps();
    if (rsx_ui_menu_open()) draw_menu();
}

} /* namespace */

/* The disc's PIC1.PNG (PS3_GAME, beside USRDIR) for the start screen. */
static void load_backdrop(const char* eboot)
{
#ifdef PORT_HAVE_PNG
    std::string dir = eboot ? eboot : "";
    for (int up = 0; up < 2; up++) {
        const size_t k = dir.find_last_of('/');
        dir = k == std::string::npos ? std::string(".") : dir.substr(0, k);
    }
    const std::string path = dir + "/PIC1.PNG";
    png_image im;
    memset(&im, 0, sizeof im);
    im.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&im, path.c_str())) {
        fprintf(stderr, "[ui] no picture at %s: the start screen has a plain backdrop\n", path.c_str());
        return;
    }
    im.format = PNG_FORMAT_RGBA;
    std::vector<unsigned char> px(PNG_IMAGE_SIZE(im));
    if (png_image_finish_read(&im, nullptr, px.data(), 0, nullptr))
        rsx_ui_set_image(px.data(), (int)im.width, (int)im.height);
    png_image_free(&im);
#else
    (void)eboot;
#endif
}

void port_ui_init(int argc, char** argv)
{
    g_boot = *port_settings_get();
    g_argc = argc; g_argv = argv;
    rsx_ui_set_draw(draw, nullptr);
    rsx_ui_set_visible(port_settings_get()->fps_counter);
    bool play_arg = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--play")) play_arg = true;
        /* --title: straight to the game's title screen (see play()). */
        if (!strcmp(argv[i], "--title")) {
            play_arg = true;
            if (backup_saves()) {
                fprintf(stderr, "[ui] the game's title screen (%s)\n", g_last_backup.c_str());
                g_savedata_title = 1;
            } else {
                fprintf(stderr, "[ui] the saves could not be backed up: the game continues the newest save\n");
            }
        }
    }
    const char* skip = getenv("PS3_SKIP_LAUNCHER");
    g_want_start = !play_arg && !(skip && skip[0] == '1') && !port_settings_get()->skip_launcher;
    if (g_want_start) load_backdrop(argc > 1 ? argv[1] : nullptr);
    /* PS3_UI_PAGE=load|new|options: open the start screen on that page (for
     * a screenshot of it). */
    if (const char* pg = getenv("PS3_UI_PAGE")) {
        if (!strcmp(pg, "load")) g_page = MENU_LOAD;
        else if (!strcmp(pg, "new")) g_page = MENU_NEW;
        else if (!strcmp(pg, "options")) g_page = MENU_OPTIONS;
    }
    if (g_want_start) g_go = 0;            /* port_ui_backend_ready decides */
}

void port_ui_backend_ready(void)
{
    if (!g_want_start) return;
    if (!rsx_ui_available()) {
        fprintf(stderr, "[ui] no UI: the game starts without its start screen\n");
        g_go = 1;
        return;
    }
    g_start = true;
    rsx_ui_set_modal(1);
    rsx_ui_set_visible(1);
}

void port_ui_wait_start(void)
{
    while (!g_go.load()) {
        /* The window went away under the start screen: nothing can press Play. */
        if (g_start.load() && !rsx_ui_available()) {
            fprintf(stderr, "[ui] the start screen is gone: the game starts\n");
            g_start = false;
            g_go = 1;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int port_ui_start_screen(void) { return g_start.load() ? 1 : 0; }

#endif

static void env_put(const char* name, const char* value)
{
#ifdef _WIN32
    _putenv_s(name, value ? value : "");   /* "" removes it */
#else
    if (value) setenv(name, value, 1); else unsetenv(name);
#endif
}

void port_resolve_aspect_auto(void)
{
    const char* rt = getenv("PS3RECOMP_RT_ASPECT");
    const char* fa = getenv("PS3RECOMP_ASPECT");
    const bool rt_auto = rt && !strcmp(rt, "auto"), fa_auto = fa && !strcmp(fa, "auto");
    if (!rt_auto && !fa_auto) return;
    int w = 0, h = 0;
#ifdef PS3RECOMP_UI
    /* The monitor the backend will open the window on (vk_window_open), at
     * its desktop mode, which is what full screen takes. Video is shut down
     * again here: the rsx thread brings it up for itself. */
    const char* win = getenv("PS3RECOMP_VK_WINDOW");
    if (win && win[0] && win[0] != '0' && SDL_InitSubSystem(SDL_INIT_VIDEO) == 0) {
        int disp = 0;
        const char* e = getenv("PS3RECOMP_VK_DISPLAY");
        if (e && *e) disp = atoi(e);
        if (disp < 0 || disp >= SDL_GetNumVideoDisplays()) disp = 0;
        SDL_DisplayMode dm;
        if (SDL_GetDesktopDisplayMode(disp, &dm) == 0) { w = dm.w; h = dm.h; }
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
#endif
    /* Wider than 16:9 by more than 1%: a 16:10 or a 16:9 monitor stays as it was. */
    const bool wide = w > 0 && h > 0 && (long long)w * 900 > (long long)h * 1616;
    char v[32];
    snprintf(v, sizeof v, "%dx%d", w, h);
    if (rt_auto) env_put("PS3RECOMP_RT_ASPECT", wide ? v : nullptr);
    if (fa_auto) env_put("PS3RECOMP_ASPECT", wide ? v : nullptr);
    if (wide) fprintf(stderr, "[ui] ultrawide: the picture at the monitor's %s\n", v);
    else if (w > 0) fprintf(stderr, "[ui] ultrawide: the monitor is %s, no wider than 16:9; the picture stays 16:9\n", v);
    else fprintf(stderr, "[ui] ultrawide: no monitor to take the shape of; the picture stays 16:9\n");
}

#ifndef PS3RECOMP_UI

void port_ui_init(int, char**) {}
void port_ui_backend_ready(void) {}
void port_ui_wait_start(void) {}
int port_ui_start_screen(void) { return 0; }

#endif
