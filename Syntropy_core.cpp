// =================================================================================
// SYNTROPY 1.4 LINUX — CORE VERSION [ C++20 | SFML 3.0 | pic © 2026 ]
//
// [RU]
// Версия приложения для запуска в стандартном окне. 
// Код самого движка симуляции (алгоритмы, математика, шейдеры) полностью идентичен 
// полной версии. Методы полноэкранного встраивания в интерактивный рабочий стол 
// являются приватными и не входят в эту сборку и не подлежат раскрытию. прошу отнестись с понимаем.
//
// [EN]
// Application version for running in a standard window.
// The core simulation engine code (algorithms, math, shaders) is completely identical 
// to the full version. Interactive desktop embedding methods are private, 
// excluded from this build, and will not be disclosed. Thank you for your understanding.
//
//
// Команда для компиляции / Compilation command:
// g++ Syntropy_core.cpp -o Syntropy -O3 -msse4.2 -std=c++20 -DSFML_STATIC -I./include \
// $(pkg-config --cflags gtkmm-3.0 x11) -ffast-math -ftree-vectorize -funroll-loops -flto=auto \
// -ffunction-sections -fdata-sections ./libsfml-graphics-s.a ./libsfml-window-s.a ./libsfml-system-s.a \
// $(pkg-config --libs gtkmm-3.0 x11) -lXrender -lXi -lXrandr -lXcursor -lGL -ludev -ldl -lpthread \
// -lfreetype -lpng -lz -ljpeg -static-libgcc -static-libstdc++ -Wl,--gc-sections -Wl,--exclude-libs,ALL
//
// * Размеры окна: параметры чуть ниже / Window dimensions: parameters just below
// =================================================================================

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#undef Always
#include <gtkmm.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xinerama.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#undef None
#undef Status
#undef Success
#undef Bool
#include "data.h"
#include <fstream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <deque>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <optional>
#include <thread>
#include <filesystem>
#include <cstring>
#include <atomic>
#include <mutex>
#include <csignal>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <string_view>
#include <xmmintrin.h>
#include <chrono>
#include <ctime>
namespace fs = std::filesystem;

// [RU] Задайте здесь желаемое разрешение окна (ширину и высоту)
// [EN] Set your preferred window resolution (width and height) here
constexpr int mw = 1600;
constexpr int mh = 900;

constexpr int MENU_SETTINGS = 0;
constexpr int MENU_RESTART = 1;
constexpr int MENU_PAUSE = 2;
constexpr int MENU_AUTOPAUSE = 3;
constexpr int MENU_PARALLAX = 4;
constexpr int MENU_MOUSE = 5;
constexpr int MENU_AUTOSTART = 6;
constexpr int MENU_SAVE_WORLD = 7;
constexpr int MENU_LANG = 8;
constexpr int MENU_ACTIVATE = 9;
constexpr int MENU_EXIT = 10;
inline std::mutex& GetLogMutex() {
  static std::mutex log_mutex;
  return log_mutex;
}
inline std::string GetLogTime() {
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(now);
  std::tm bt;
  localtime_r(&t, &bt);
  char b[32];
  std::strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S", &bt);
  return std::string(b);
}
#define LOG_WRITE(level, tag, msg)                                                                  \
  do {                                                                                              \
    std::lock_guard<std::mutex> lock(GetLogMutex());                                                \
    sf::err() << "[" << GetLogTime() << "] [" << level << "] [" << tag << "] " << msg << std::endl; \
  } while (0)
#define LOG_INFO(tag, msg) LOG_WRITE("INFO", tag, msg)
#define LOG_WARN(tag, msg) LOG_WRITE("WARN", tag, msg)
#define LOG_ERROR(tag, msg) LOG_WRITE("ERROR", tag, msg)
inline void LinuxCrashHandler(int signum) {
  std::string reason = "UNKNOWN_SIGNAL";
  if (signum == SIGSEGV)
    reason = "SIGSEGV (Segmentation Fault / Invalid memory access)";
  else if (signum == SIGFPE)
    reason = "SIGFPE (Arithmetic Exception / Divide by zero)";
  else if (signum == SIGILL)
    reason = "SIGILL (Illegal Instruction / Unsupported CPU opcode)";
  else if (signum == SIGABRT)
    reason = "SIGABRT (Aborted by system or assert)";
  do {
    std::lock_guard<std::mutex> lock(GetLogMutex());
    sf::err() << "[" << GetLogTime() << "] [CRITICAL] [System] Application crashed! Signal: "
              << signum << " -> " << reason << std::endl;
  } while (0);
  std::signal(signum, SIG_DFL);
  std::raise(signum);
}
constexpr std::string_view raysShaderCode = R"(
#version 130

uniform vec2 u_resolution;
uniform vec4 u_times;
uniform vec3 color;

	void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution;
    
    vec2 pos = vec2(sin(u_times.x) - 0.6, 5.0);

    vec2 dir = uv - pos;
    float angle = atan(dir.y, dir.x);
    
    float s1 = sin(angle * 90.0 - u_times.y) * 0.5 + 0.5;
    float s2 = sin(angle * 73.0 + u_times.z) * 0.5 + 0.5;
    float s3 = sin(angle * 160.0 - u_times.w) * 0.5 + 0.5;
    
    s1 *= s1; s1 *= s1; s1 *= s1;
    s2 *= s2; s2 *= s2;
    s3 *= s3; s3 *= s3; s3 *= s3;
    
    float x = uv.x;
    float edgeMask = smoothstep(0.0, 0.16, x * (1.0 - x));
    float finalRay = (s1 + s2 * 0.7 + s3 * 0.5) * uv.y * edgeMask;
    
    gl_FragColor = vec4(color * finalRay, 1.0);
}
)";
constexpr std::string_view vignetteShaderCode = R"(
#version 130

uniform vec2 u_resolution;

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution;
    
    vec2 p = uv - vec2(0.5, 1.1);
    p.x *= 2.2;
    float d2 = dot(p, p);
    float vig = clamp((d2 - 0.64) / (1.96 - 0.64), 0.0, 1.0);
    vig = vig * vig * (3.0 - 2.0 * vig);
    
    float noise = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
    vig += (noise - 0.5) * 0.04;
    
    gl_FragColor = vec4(0.0, 0.0, 0.0, clamp(vig * 0.6, 0.0, 1.0));
}
)";
constexpr std::string_view downSrc = R"(
#version 130

uniform sampler2D u_tex;
uniform vec2 u_res;

void main() {
    vec2 uv = gl_FragCoord.xy / u_res;
    vec2 d = 1.0 / u_res;
    
    vec4 col = texture2D(u_tex, uv + vec2(-d.x, -d.y));
    col     += texture2D(u_tex, uv + vec2( d.x, -d.y));
    col     += texture2D(u_tex, uv + vec2(-d.x,  d.y));
    col     += texture2D(u_tex, uv + vec2( d.x,  d.y));
    
    gl_FragColor = col * 0.25;
}
)";
constexpr std::string_view upSrc = R"(
#version 130

uniform sampler2D u_tex;
uniform vec2 u_res;
uniform float glow;

void main() {
    vec2 uv = gl_FragCoord.xy / u_res;
    vec2 d = 2.0 / u_res;
    
    vec4 col = texture2D(u_tex, uv) * 0.5;
    float stepFactor = 0.125;
    
    col += texture2D(u_tex, uv + vec2(-d.x, -d.y)) * stepFactor;
    col += texture2D(u_tex, uv + vec2( d.x, -d.y)) * stepFactor;
    col += texture2D(u_tex, uv + vec2(-d.x,  d.y)) * stepFactor;
    col += texture2D(u_tex, uv + vec2( d.x,  d.y)) * stepFactor;
    
    gl_FragColor = vec4(col.rgb * glow, col.a * glow);
}
)";
constexpr std::string_view upSrcChrom = R"(
#version 130

uniform sampler2D u_tex;
uniform vec2 u_res;
uniform float glow;
uniform float chromAb; 

void main() {
    vec2 uv = gl_FragCoord.xy / u_res;
    vec2 d = 2.0 / u_res;
    
    vec4 blurCol = texture2D(u_tex, uv) * 0.5;
    float stepFactor = 0.125;
    blurCol += texture2D(u_tex, uv + vec2(-d.x, -d.y)) * stepFactor;
    blurCol += texture2D(u_tex, uv + vec2( d.x, -d.y)) * stepFactor;
    blurCol += texture2D(u_tex, uv + vec2(-d.x,  d.y)) * stepFactor;
    blurCol += texture2D(u_tex, uv + vec2( d.x,  d.y)) * stepFactor;
    
    vec2 distortDir = uv - vec2(0.5, 0.5);
    float dist = length(distortDir);
    float smoothFactor = (0.3 + dist * 0.7) * 0.2;
    vec2 radialShift = distortDir * (smoothFactor * chromAb);
    vec2 axialShift = vec2(0.0003, 0.0002) * chromAb;
    vec2 finalShift = radialShift + axialShift;
    const int SAMPLES = 6;
    vec3 chromRGB = vec3(0.0);
    vec3 wSum = vec3(0.0);
    for (int i = 0; i < SAMPLES; i++) {
        float t = float(i) / float(SAMPLES - 1);
        vec3 w = clamp(1.0 - abs(vec3(t) - vec3(0.0, 0.5, 1.0)) * 2.0, 0.0, 1.0);
        wSum += w; 
        vec2 sampleUV = uv + finalShift * (t - 0.5);
        vec3 texColor = texture2D(u_tex, sampleUV).rgb;
        chromRGB += texColor * w;
    }
    vec3 finalRGB = chromRGB / wSum;
    vec3 col = mix(blurCol.rgb, finalRGB, blurCol.a * 0.95);
    gl_FragColor = vec4(col * glow, blurCol.a * glow);
}
)";
constexpr std::string_view fastCellShaderCode = R"(
#version 130
uniform sampler2D u_tex;
uniform float isMask;

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    float isParticle = 1.0 - step(0.001, uv.y);
    vec4 tex = texture2D(u_tex, uv);
    float alphaMask = (tex.a > 0.001) ? 1.0 : 0.0;
    float cellAlpha = tex.a * gl_Color.a * alphaMask;
    float particleAlpha = gl_Color.a;
    float finalAlpha = mix(cellAlpha, particleAlpha, isParticle);
    float dimFactor = max(gl_Color.r, max(gl_Color.g, gl_Color.b));
    if (dimFactor < 0.001) dimFactor = 0.0;

    vec3 base = tex.rgb * gl_Color.rgb * gl_Color.a * 1.8;
				vec3 spec = max(vec3(0.0), tex.rgb - 0.75) * 3.5 * dimFactor * gl_Color.a;
    vec3 cellColor = mix(base + spec, gl_Color.rgb, step(0.5, isMask));
    vec3 finalRGB = mix(cellColor, gl_Color.rgb, isParticle);
   
    gl_FragColor = vec4(finalRGB, finalAlpha);
}
)";
constexpr std::string_view cellShaderCode = R"(
#version 130
uniform vec2 size;
uniform float round;
uniform float bevel;
uniform float convex_power;

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec2 p = (uv - 0.5) * size;
    float t = clamp(round / 100.0, 0.0, 1.0);
    float maxR = size.x * 0.5;
    float d;
    if (t <= 0.5) {
        float r = clamp(t * 2.0 * (maxR - 0.5), 0.0, maxR - 0.5);
        vec2 q = abs(p) - (size * 0.5) + r + 1.0;
        d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
    } else {
        float r = maxR - 0.5;
        vec2 q = abs(p) - (size * 0.5) + r + 1.0;
        float dCircle = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
        float dRhomb = (abs(p.x) + abs(p.y)) - maxR;
        float blend = (t - 0.5) * 2.0;
        d = mix(dCircle, dRhomb, blend);
    }
    d = min(d, maxR - 1.0);
    float alpha = max(0.0, min(1.0, 0.5 - d));
    float stepFactor = (alpha > 0.001) ? 1.0 : 0.0;
    vec3 col = vec3(0.5);
    float pwr = convex_power * 0.01;
    if (pwr > 0.0) {
        float b_width = max(0.001, size.x * bevel);
        float h = max(0.0, min(1.0, -d / b_width));
        float facet = (1.0 - h);
        float facet2 = facet * facet;
        float facet4 = facet2 * facet2;
        vec2 n = p / (length(p) + 10.0);
        float slope = (n.x + n.y) * -0.707;
        col += slope * facet * pwr * 0.8;
        float s2 = slope * 1.5; s2 = max(0.0, s2);
		float s4 = s2 * s2;
		float s8 = s4 * s4;
        col += vec3(s8 * facet4 * 2.5 * pwr);
        col += facet2 * 0.3 * pwr;
    }
    gl_FragColor = vec4(col * alpha * stepFactor, alpha * stepFactor);
}
)";
bool has(const std::string& cmd) {
  return std::system(("which " + cmd + " > /dev/null 2>&1").c_str()) == 0;
}
sf::Color fromHSV(float h, float s, float v) {
  auto f = [&](float n) {
    float k = std::fmod(n + h * 6.0f, 6.0f);
    float min_val = std::min({k, 4.0f - k, 1.0f});
    float res = v - v * s * std::max(0.0f, min_val);
    return static_cast<uint8_t>(std::clamp(res * 255.0f, 0.0f, 255.0f));
  };
  return sf::Color(f(5.0f), f(3.0f), f(1.0f));
}
std::mutex x11_mutex;
std::atomic<bool> running{true};
std::atomic<bool> showSettings{false};
std::atomic<bool> needRestart{false};
std::atomic<bool> keepColorsOnRestart{false};
std::atomic<bool> isPaused{false};
std::atomic<bool> autoPaused{false};
std::atomic<bool> isRussian{true};
std::atomic<int> frames_to_capture{0};
bool wasP = false;
bool lastMouseState = false;
static bool needAtlasUpdate = false;
sf::Font globalFont;
Atom ATOM_ACTIVE_WINDOW = 0;
Atom ATOM_WM_STATE = 0;
Atom ATOM_WM_FULLSCREEN = 0;
Atom ATOM_WM_TYPE = 0;
Atom ATOM_WM_DESKTOP = 0;
sf::String toUtf8(const std::string& s) {
  return sf::String::fromUtf8(s.begin(), s.end());
}
sf::String toUtf8(const std::wstring& s) {
  return sf::String(s);
}
const std::wstring L(const std::wstring& ru, const std::wstring& en) {
  return isRussian.load() ? ru : en;
}
struct PrecalculatedUV {
  float tx0, tx1;
  float ty0, ty1;
};
std::string get_cfg_path(std::string_view name) {
  static std::string basePath = "";
  if (basePath.empty()) {
    const char* home = getenv("HOME");
    fs::path p = (home) ? fs::path(home) / ".config" / "Syntropy" : fs::path("./Syntropy");
    std::error_code ec;
    if (!fs::exists(p, ec)) {
      fs::create_directories(p, ec);
    }
    basePath = p.string();
  }
  return (fs::path(basePath) / name).string();
}
void setIcon(sf::RenderWindow& w) {
  sf::Image img;
  if (img.loadFromMemory(icon_png, icon_png_len)) {
    w.setIcon(img.getSize(), img.getPixelsPtr());
  }
}
bool IsAutoStartEnabled() {
  const char* home = getenv("HOME");
  if (!home) return false;
  return fs::exists(fs::path(home) / ".config" / "autostart" / "Syntropy.desktop");
}
void SetAutoStart(bool e) {
  const char* home = getenv("HOME");
  if (!home) return;
  fs::path p = fs::path(home) / ".config" / "autostart" / "Syntropy.desktop";
  if (e) {
    char buffer[1024];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len == -1) return;
    buffer[len] = '\0';
    std::string iconPath = get_cfg_path("icon.png");
    if (!fs::exists(iconPath)) {
      std::ofstream o(iconPath, std::ios::binary);
      o.write(reinterpret_cast<const char*>(icon_png), icon_png_len);
    }
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream o(p);
    if (o.is_open()) {
      o << "[Desktop Entry]\n"
        << "Type=Application\n"
        << "Name=Syntropy\n"
        << "Exec=sh -c 'sleep 3 && \"" << buffer << "\"'\n"
        << "Icon=" << iconPath << "\n"
        << "Terminal=false\n"
        << "Categories=Utility;\n";
    }
  } else {
    if (fs::exists(p)) {
      std::error_code ec;
      fs::remove(p, ec);
    }
  }
}
struct Config {
  int speed = 250;
  int aggression = 4;
  float mutation_chance = 25.0f;
  int max_teams = 3;
  int fps = 60;
  int res = 20;
  int gap = 2;
  int round = 15;
  float bevel_ratio = 0.15f;
  int convex_power = 50;
  float glow_sigma = 30.0f;
  bool only_glow = false;
  bool chromatic_aberration = true;
  int fade_ms = 500;
  bool dust = true;
  bool alpha_fade = true;
  float bgValue = 20.0f;
  float bgSaturation = 70.0f;
  bool parallax_enabled = true;
  bool mouse_enabled = true;
  bool auto_pause = false;
  bool save_world = true;
};
class SyncedLifeEngine {
 private:
  std::vector<int> teamVotes;
  std::vector<int> teamVotesGen;
  std::vector<int> aL_buf;
  std::vector<int> teamPops;
  std::vector<int> teamLocs;
  struct CellChange {
    int idx;
    int8_t alive;
    int team;
  };
  std::vector<CellChange> changes;
  std::vector<int> ext;
  struct ItemInfo {
    int x, y, id, idx;
  };
  std::vector<ItemInfo> localAnim;
  struct AlignmentDust {
    std::vector<float> posX;
    std::vector<float> posY;
    std::vector<float> velX;
    std::vector<float> velY;
    std::vector<float> phase;
    std::vector<float> life;
    std::vector<uint32_t> color;
    std::vector<uint8_t> type;
    void clear() {
      posX.clear();
      posY.clear();
      velX.clear();
      velY.clear();
      phase.clear();
      life.clear();
      color.clear();
      type.clear();
    }
    void pop_back() {
      posX.pop_back();
      posY.pop_back();
      velX.pop_back();
      velY.pop_back();
      phase.pop_back();
      life.pop_back();
      color.pop_back();
      type.pop_back();
    }
    size_t size() const { return posX.size(); }
    bool empty() const { return posX.empty(); }
  };
  AlignmentDust simdParticles;
  std::vector<sf::Vertex> particlesCPU;
  size_t MAX_PARTICLES;
  size_t lastActiveVerticesCount = 0;
  sf::Vector2f bloomOffset{0.f, 0.f};
  Config cfg;
  sf::Clock sessionClock, colorUpdateClock, globalClock, logicClock;
  sf::VertexBuffer mainVB;
  sf::VertexBuffer staticVB;
  std::vector<sf::Vertex> staticVertexCPU;
  bool staticBufferNeedsUpdate = true;
  std::vector<sf::Vertex> vertexCPU;
  std::vector<float> preX, preY;
  float sinTable[1024];
  static constexpr float RAD_TO_INDEX = 1023.0f / 6.2831853f;
  inline float fastSin(float x) const {
    if (!std::isfinite(x)) return 0.0f;
    uint32_t idx = static_cast<uint32_t>(std::abs(x) * RAD_TO_INDEX) & 1023;
    return sinTable[idx];
  }
  inline float fastCos(float x) const {
    return fastSin(x + 1.570796f);
  }
  float currentPadF = 0.0f;
  const float K_PARA = 18.0f;

 public:
  enum SliderID {
    ID_NONE,
    ID_SPD,
    ID_AGGR,
    ID_MUT,
    ID_TEAMS,
    ID_FPS,
    ID_RES,
    ID_GAP,
    ID_ROUND,
    ID_BEVEL,
    ID_CONVEX,
    ID_GLOW,
    ID_FADE,
    ID_BGV,
    ID_BGS
  };
  SliderID activeSliderId = ID_NONE;
  size_t activeAnimations = 0;
  int cols = 0, rows = 0, totalW = 0, totalH = 0, tidCounter = 1;
  uint32_t currentGen = 1;
  bool glowWeightsNeedUpdate = true;
  bool needsResort = true;
  bool worldChanged = true;
  std::vector<int8_t> aliveMap;
  std::vector<int32_t> teamMap;
  std::vector<int64_t> birthMap;
  std::vector<int8_t>& getAliveMap() { return aliveMap; }
  std::vector<int32_t>& getTeamMap() { return teamMap; }
  std::vector<int64_t>& getBirthMap() { return birthMap; }
  struct ColorCache {
    sf::Color main;
    uint32_t packed;
    sf::Color high, low;
  };
  struct CellEffect {
    int teamId = 0;
    int64_t startTimeMs = -1;
  };
  struct ActiveCoord {
    int idx;
    int x;
    int y;
  };
  float cachedPadX = 0, cachedPadY = 0;
  std::vector<ColorCache> colorCache;
  std::vector<sf::Color> teamColors;
  std::vector<CellEffect> fadeStates, spawnStates;
  std::vector<int> activeCells, nextActiveCells;
  std::vector<int> freeTeamIDs;
  std::vector<uint32_t> cellGenList, teamActiveGen;
  std::vector<std::deque<int>> teamHistory;
  std::vector<int> teamStasisCount;
  std::mt19937 gen{std::random_device{}()};
  sf::RenderTexture cellAtlasRT;
  sf::Shader fastCellShader;
  sf::Shader cellShader, raysShader, shDown, shUp, shUpChrom, vignetteShader;
  sf::RectangleShape raysRect, vigRect;
  sf::Texture sinTex;
  std::vector<sf::RenderTexture> mips;
  sf::RenderTexture rt2;
  sf::Vector2f targetParallax{0.f, 0.f}, parallaxOffset{0.f, 0.f};
  float currentBgHue = 0.0f, internalTime = 0.f;
  sf::Color currentRuntimeBgColor;
  float lastHue = 0.0f, targetBgHue = 0.0f;
  bool vLoaded = false;
  void generateCellAtlas() {
    int safeRes = std::max(4, cfg.res);
    unsigned int aS = static_cast<unsigned int>(safeRes) + 4;
    if (cellAtlasRT.getTexture().getSize().x != aS * 2) {
      if (!cellAtlasRT.resize({aS * 2, aS})) return;
      cellAtlasRT.setSmooth(true);
    }
    sf::VertexArray canvas(sf::PrimitiveType::TriangleStrip, 4);
    cellShader.setUniform("size", sf::Vector2f((float)aS, (float)aS));
    cellShader.setUniform("round", (float)cfg.round);
    cellShader.setUniform("bevel", (float)cfg.bevel_ratio);
    cellShader.setUniform("convex_power", (float)cfg.convex_power);
    cellAtlasRT.clear(sf::Color::Transparent);
    sf::RenderStates states;
    states.shader = &cellShader;
    states.blendMode = sf::BlendNone;
    canvas[0].position = {0.f, 0.f};
    canvas[0].texCoords = {0.f, 0.f};
    canvas[1].position = {(float)aS, 0.f};
    canvas[1].texCoords = {1.f, 0.f};
    canvas[2].position = {0.f, (float)aS};
    canvas[2].texCoords = {0.f, 1.f};
    canvas[3].position = {(float)aS, (float)aS};
    canvas[3].texCoords = {1.f, 1.f};
    cellAtlasRT.draw(canvas, states);
    cellShader.setUniform("bevel", 0.0f);
    cellShader.setUniform("convex_power", 0.0f);
    for (size_t i = 0; i < 4; i++) canvas[i].position.x += (float)aS;
    cellAtlasRT.draw(canvas, states);
    cellShader.setUniform("bevel", (float)cfg.bevel_ratio);
    cellShader.setUniform("convex_power", (float)cfg.convex_power);
    cellAtlasRT.display();
  }
  float getCurrentDim() {
    return 1.0f;
  }
  inline size_t idx(int x, int y) const { return (size_t)y * cols + x; }
  sf::Color adj(sf::Color c, int a) {
    return sf::Color(
        static_cast<uint8_t>(std::clamp(static_cast<int>(c.r) + a, 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(c.g) + a, 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(c.b) + a, 0, 255)),
        c.a);
  }
  sf::Vector2f getCameraOffset() const {
    float t = internalTime;
    if (!std::isfinite(t)) return parallaxOffset;
    return {fastSin(t * 0.4f) * 6.f + parallaxOffset.x,
            fastCos(t * 0.3f) * 4.f + parallaxOffset.y};
  }
  void saveWorldState() {
    if (!cfg.save_world) return;
    std::ofstream f(get_cfg_path("world.dat"), std::ios::binary);
    if (!f.is_open()) return;
    f.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
    f.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
    uint32_t aliveCount = 0;
    for (int8_t a : aliveMap)
      if (a) aliveCount++;
    f.write(reinterpret_cast<const char*>(&aliveCount), sizeof(aliveCount));
    uint8_t pFlag = cfg.parallax_enabled ? 1 : 0;
    f.write(reinterpret_cast<const char*>(&pFlag), sizeof(pFlag));
    f.write(reinterpret_cast<const char*>(&tidCounter), sizeof(tidCounter));
    uint32_t safeLimit = static_cast<uint32_t>(std::min(static_cast<size_t>(tidCounter), teamColors.size()));
    f.write(reinterpret_cast<const char*>(&safeLimit), sizeof(safeLimit));
    for (size_t i = 0; i < safeLimit; i++) {
      sf::Color c = teamColors[i];
      f.write(reinterpret_cast<const char*>(&c.r), 1);
      f.write(reinterpret_cast<const char*>(&c.g), 1);
      f.write(reinterpret_cast<const char*>(&c.b), 1);
      f.write(reinterpret_cast<const char*>(&c.a), 1);
    }
    size_t mapsSize = std::min({aliveMap.size(), teamMap.size(), birthMap.size()});
    for (size_t i = 0; i < mapsSize; i++) {
      if (aliveMap[i]) {
        uint32_t safeIdx = static_cast<uint32_t>(i);
        f.write(reinterpret_cast<const char*>(&safeIdx), sizeof(safeIdx));
        f.write(reinterpret_cast<const char*>(&teamMap[i]), sizeof(teamMap[i]));
        f.write(reinterpret_cast<const char*>(&birthMap[i]), sizeof(birthMap[i]));
      }
    }
  }
  bool loadWorldState() {
    if (!cfg.save_world) return false;
    std::ifstream f(get_cfg_path("world.dat"), std::ios::binary);
    if (!f.is_open()) return false;
    int savedCols = 0, savedRows = 0;
    if (!f.read(reinterpret_cast<char*>(&savedCols), sizeof(savedCols)) ||
        !f.read(reinterpret_cast<char*>(&savedRows), sizeof(savedRows))) {
      f.close();
      return false;
    }
    if (savedCols != cols || savedRows != rows) {
      f.close();
      return false;
    }
    uint32_t aliveCount = 0;
    if (!f.read(reinterpret_cast<char*>(&aliveCount), sizeof(aliveCount))) {
      f.close();
      return false;
    }
    uint8_t pFlag = 0;
    if (!f.read(reinterpret_cast<char*>(&pFlag), sizeof(pFlag))) {
      f.close();
      return false;
    }
    cfg.parallax_enabled = (pFlag == 1);
    if (!f.read(reinterpret_cast<char*>(&tidCounter), sizeof(tidCounter))) {
      f.close();
      return false;
    }
    if (tidCounter <= 0 || tidCounter > 100000) {
      f.close();
      return false;
    }
    ensureTeamCapacity(tidCounter);
    if (teamColors.size() < static_cast<size_t>(tidCounter)) {
      f.close();
      return false;
    }
    uint32_t savedColorsCount32 = 0;
    if (!f.read(reinterpret_cast<char*>(&savedColorsCount32), sizeof(savedColorsCount32))) {
      f.close();
      return false;
    }
    size_t savedColorsCount = static_cast<size_t>(savedColorsCount32);
    if (savedColorsCount > teamColors.size()) {
      savedColorsCount = teamColors.size();
    }
    for (size_t i = 0; i < savedColorsCount; i++) {
      uint8_t r, g, b, a;
      if (!f.read(reinterpret_cast<char*>(&r), 1) ||
          !f.read(reinterpret_cast<char*>(&g), 1) ||
          !f.read(reinterpret_cast<char*>(&b), 1) ||
          !f.read(reinterpret_cast<char*>(&a), 1)) {
        f.close();
        return false;
      }
      if (i < teamColors.size()) {
        teamColors[i] = sf::Color(r, g, b, a);
      }
    }
    aliveMap.assign(aliveMap.size(), 0);
    teamMap.assign(teamMap.size(), 0);
    birthMap.assign(birthMap.size(), 0);
    nextActiveCells.clear();
    std::vector<int> counts(teamColors.size(), 0);
    for (uint32_t i = 0; i < aliveCount; i++) {
      int idx_l;
      int32_t team;
      int64_t birth;
      if (!f.read(reinterpret_cast<char*>(&idx_l), sizeof(idx_l)) ||
          !f.read(reinterpret_cast<char*>(&team), sizeof(team)) ||
          !f.read(reinterpret_cast<char*>(&birth), sizeof(birth))) {
        f.close();
        return false;
      }
      if (idx_l >= 0 &&
          static_cast<size_t>(idx_l) < aliveMap.size() &&
          static_cast<size_t>(idx_l) < teamMap.size() &&
          static_cast<size_t>(idx_l) < birthMap.size() &&
          team >= 0 &&
          static_cast<size_t>(team) < teamColors.size()) {
        aliveMap[idx_l] = 1;
        teamMap[idx_l] = team;
        birthMap[idx_l] = birth;
        if (team > 0) counts[team]++;
        markActive(idx_l);
      }
    }
    f.close();
    int maxP = 0, domT = -1;
    for (int i = 1; i < tidCounter; i++) {
      if (counts[i] > maxP) {
        maxP = counts[i];
        domT = i;
      }
    }
    if (domT != -1 && static_cast<size_t>(domT) < teamColors.size()) {
      currentBgHue = getH(teamColors[domT]);
    }
    currentRuntimeBgColor = fromHSV(currentBgHue, cfg.bgSaturation / 100.f, cfg.bgValue / 100.f);
    updateColorCache(getCurrentDim());
    return aliveCount > 0;
  }
  SyncedLifeEngine(int w, int h) : totalW(w), totalH(h) {
    tidCounter = 1;
    ensureTeamCapacity(4100);
    for (int i = 0; i < 1024; i++) {
      sinTable[i] = std::sin(i * 6.2831853f / 1024.f);
    }
    std::vector<uint8_t> pixels(1024 * 4);
    for (int i = 0; i < 1024; i++) {
      uint8_t v = static_cast<uint8_t>((sinTable[i] * 0.5f + 0.5f) * 255.f);
      pixels[i * 4] = v;
      pixels[i * 4 + 1] = v;
      pixels[i * 4 + 2] = v;
      pixels[i * 4 + 3] = 255;
    }
    if (!sinTex.resize({1024, 1})) {
      LOG_ERROR("Engine", "Failed to allocate memory for sinTex texture. Aborting initialization.");
      return;
    }
    sinTex.update(pixels.data());
    sinTex.setSmooth(true);
    sinTex.setRepeated(true);
    if (sf::Shader::isAvailable()) {
      bool allLoaded = true;
      if (!cellShader.loadFromMemory(std::string(cellShaderCode), sf::Shader::Type::Fragment)) {
        LOG_ERROR("Shader", "Failed to compile cellShader fragment source.");
        allLoaded = false;
      }
      if (!fastCellShader.loadFromMemory(std::string(fastCellShaderCode), sf::Shader::Type::Fragment)) {
        LOG_ERROR("Shader", "Failed to compile fastCellShader fragment source.");
        allLoaded = false;
      }
      if (!raysShader.loadFromMemory(std::string(raysShaderCode), sf::Shader::Type::Fragment)) {
        LOG_ERROR("Shader", "Failed to compile raysShader fragment source.");
        allLoaded = false;
      }
      if (!shDown.loadFromMemory(std::string(downSrc), sf::Shader::Type::Fragment)) {
        LOG_ERROR("Shader", "Failed to compile shDown (Bloom Downsample) fragment source.");
        allLoaded = false;
      }
      if (!shUp.loadFromMemory(std::string(upSrc), sf::Shader::Type::Fragment)) {
        LOG_ERROR("Shader", "Failed to compile shUp (Bloom Upsample) fragment source.");
        allLoaded = false;
      }
      if (!shUpChrom.loadFromMemory(std::string(upSrcChrom), sf::Shader::Type::Fragment)) {
        LOG_ERROR("Shader", "Failed to compile shUpChrom (Chromatic Upsample) fragment source.");
        allLoaded = false;
      }
      if (vignetteShader.loadFromMemory(std::string(vignetteShaderCode), sf::Shader::Type::Fragment)) {
        vLoaded = true;
      } else {
        LOG_WARN("Shader", "Failed to compile vignetteShader fragment source. Continuing without vignette.");
      }
      if (!allLoaded) {
        LOG_ERROR("Engine", "Critical: One or more essential shaders failed to compile. Visual anomalies may occur.");
      }
    } else {
      LOG_ERROR("Engine", "Critical: Shaders are not supported on this hardware/driver configuration.");
    }
    vigRect.setSize({(float)w, (float)h});
    raysRect.setSize({(float)w, (float)h});
    unsigned int rt2W = std::max(1u, static_cast<unsigned int>(w / 4u));
    unsigned int rt2H = std::max(1u, static_cast<unsigned int>(h / 4u));
    if (!rt2.resize({rt2W, rt2H})) {
      LOG_ERROR("Render", "Failed to initialize or resize rt2 RenderTexture target.");
    }
    rt2.setSmooth(true);
    rt2.display();
    loadConfig();
    int r = std::max(1, cfg.res);
    float pF = cfg.parallax_enabled ? ((std::ceil((totalW / 2.f) / K_PARA) / r) + 2) * r : 2.f * r;
    currentPadF = pF;
    unsigned int tW = static_cast<unsigned int>(totalW) + static_cast<unsigned int>(pF * 2.f);
    unsigned int tH = static_cast<unsigned int>(totalH) + static_cast<unsigned int>(pF * 2.f);
    mips.clear();
    for (int i = 0; i < 4; i++) {
      mips.emplace_back();
      unsigned int resW = (i == 0) ? tW : (i == 1) ? tW / 4
                                                   : tW / (4 * (1 << (i - 1)));
      unsigned int resH = (i == 0) ? tH : (i == 1) ? tH / 4
                                                   : tH / (4 * (1 << (i - 1)));
      if (!mips.back().resize({std::max(1u, resW), std::max(1u, resH)})) {
        LOG_ERROR("Render", "Failed to allocate RenderTexture memory for Bloom mipmaps cascade.");
      }
      mips.back().setSmooth(true);
      mips.back().display();
    }
    size_t initialV = 1000000;
    if (!mainVB.create(initialV)) {
      LOG_ERROR("Render", "Failed to allocate GPU memory for main dynamic VertexBuffer.");
    }
    mainVB.setPrimitiveType(sf::PrimitiveType::Triangles);
    mainVB.setUsage(sf::VertexBuffer::Usage::Stream);
    vertexCPU.resize(initialV);
    size_t initialStaticV = 200000;
    if (!staticVB.create(initialStaticV)) {
      LOG_ERROR("Render", "Failed to allocate GPU memory for static VertexBuffer layer.");
    }
    staticVB.setPrimitiveType(sf::PrimitiveType::Triangles);
    staticVB.setUsage(sf::VertexBuffer::Usage::Static);
    staticVertexCPU.resize(initialStaticV);
    teamVotes.resize(4100, 0);
    teamVotesGen.resize(4100, 0);
    teamPops.resize(4100, 0);
    teamLocs.resize(4100, -1);
    aL_buf.reserve(64);
    changes.reserve(10000);
    ext.reserve(60000);
    localAnim.reserve(5000);
    if (w <= 0 || h <= 0) {
      w = 1920;
      h = 1080;
    }
    size_t calculatedLimit = static_cast<size_t>((w * h) / 500);
    MAX_PARTICLES = std::clamp(calculatedLimit, static_cast<size_t>(500), static_cast<size_t>(50000));
    simdParticles.clear();
    simdParticles.posX.reserve(MAX_PARTICLES);
    simdParticles.posY.reserve(MAX_PARTICLES);
    simdParticles.velX.reserve(MAX_PARTICLES);
    simdParticles.velY.reserve(MAX_PARTICLES);
    simdParticles.phase.reserve(MAX_PARTICLES);
    simdParticles.life.reserve(MAX_PARTICLES);
    simdParticles.color.reserve(MAX_PARTICLES);
    simdParticles.type.reserve(MAX_PARTICLES);
    particlesCPU.assign(MAX_PARTICLES * 6, sf::Vertex{});
    glowWeightsNeedUpdate = true;
    restart(true, false);
  }
  void ensureTeamCapacity(int id) {
    if (id < 0) return;
    if (static_cast<size_t>(id) >= teamColors.size()) {
      size_t n = static_cast<size_t>(id) + 500;
      teamColors.resize(n, sf::Color::Black);
      colorCache.resize(n);
      teamActiveGen.resize(n, 0);
      teamHistory.resize(n);
      teamStasisCount.resize(n, 0);
      teamVotes.resize(n, 0);
      teamVotesGen.resize(n, 0);
      teamPops.resize(n, 0);
      teamLocs.resize(n, -1);
    }
  }
  float getH(sf::Color c) {
    float r = c.r / 255.f, g = c.g / 255.f, b = c.b / 255.f;
    float mx = std::max({r, g, b}), mn = std::min({r, g, b}), d = mx - mn;
    if (d < 0.001f) return 0.f;
    float h = (mx == r) ? (g - b) / d + (g < b ? 6.f : 0.f) : (mx == g) ? (b - r) / d + 2.f
                                                                        : (r - g) / d + 4.f;
    return h / 6.f;
  }
  float getBestUniqueHue(const std::vector<int>& active) {
    std::vector<float> usedHues;
    usedHues.reserve(active.size() + 1);
    for (int id : active) {
      if (id > 0 && static_cast<size_t>(id) < teamColors.size()) {
        usedHues.push_back(getH(teamColors[id]));
      }
    }
    usedHues.push_back(currentBgHue);
    float bestH = fmod(lastHue + 0.618033f, 1.0f);
    float maxMinDist = -1.0f;
    std::uniform_real_distribution<float> d(0.f, 1.f);
    for (int i = 0; i < 50; i++) {
      float candidate = (i == 0) ? bestH : d(gen);
      float minDist = 2.0f;
      for (float h : usedHues) {
        float diff = std::abs(candidate - h);
        if (diff > 0.5f) diff = 1.0f - diff;
        if (diff < minDist) minDist = diff;
      }
      if (minDist > maxMinDist) {
        maxMinDist = minDist;
        bestH = candidate;
      }
      if (maxMinDist > 0.12f) break;
    }
    return lastHue = bestH;
  }
  void collectGarbageTeams() {
    std::vector<bool> alreadyFree(tidCounter, false);
    for (int id : freeTeamIDs) {
      if (id < tidCounter) alreadyFree[id] = true;
    }
    for (int t = 2; t < tidCounter; ++t) {
      if (static_cast<size_t>(t) < teamActiveGen.size() && currentGen > 100 && teamActiveGen[t] < (currentGen - 100)) {
        if (!alreadyFree[t]) {
          freeTeamIDs.push_back(t);
          if (static_cast<size_t>(t) < teamHistory.size()) teamHistory[t].clear();
          if (static_cast<size_t>(t) < teamStasisCount.size()) teamStasisCount[t] = 0;
          alreadyFree[t] = true;
        }
      }
    }
  }
  void markActive(int idx_l) {
    if (idx_l < 0 || static_cast<size_t>(idx_l) >= aliveMap.size()) return;
    if (cellGenList[idx_l] != currentGen) {
      cellGenList[idx_l] = currentGen;
      nextActiveCells.push_back(idx_l);
    }
    int cx = idx_l % cols;
    int cy = idx_l / cols;
    int y_u = (cy == 0 ? rows - 1 : cy - 1) * cols;
    int y_s = cy * cols;
    int y_d = (cy == rows - 1 ? 0 : cy + 1) * cols;
    int x_l = (cx == 0 ? cols - 1 : cx - 1);
    int x_r = (cx == cols - 1 ? 0 : cx + 1);
    const size_t nb[8] = {
        static_cast<size_t>(y_u + x_l), static_cast<size_t>(y_u + cx), static_cast<size_t>(y_u + x_r),
        static_cast<size_t>(y_s + x_l), static_cast<size_t>(y_s + x_r),
        static_cast<size_t>(y_d + x_l), static_cast<size_t>(y_d + cx), static_cast<size_t>(y_d + x_r)};
    for (int k = 0; k < 8; k++) {
      size_t nI = nb[k];
      if (nI < cellGenList.size() && cellGenList[nI] != currentGen) {
        cellGenList[nI] = currentGen;
        nextActiveCells.push_back(static_cast<int>(nI));
      }
    }
  }
  void updateColorCache(float gDim) {
    ensureTeamCapacity(tidCounter);
    uint8_t alpha = 255;
    int safeLimit = std::min(tidCounter, static_cast<int>(teamColors.size()));
    for (int i = 0; i < safeLimit; i++) {
      sf::Color c = teamColors[i];
      if (c == sf::Color::Black && i > 0) {
        colorCache[i] = {sf::Color::Black, 0, sf::Color::Black, sf::Color::Black};
        continue;
      }
      c.a = alpha;
      colorCache[i] = {c, c.toInteger(), adj(c, cfg.convex_power), adj(c, -cfg.convex_power)};
    }
  }
  void restart(bool keepColors, bool forceReset) {
    int oldCols = cols;
    int oldRows = rows;
    int oldRes = std::max(1, cfg.res);
    struct TempCell {
      int x, y, team;
      int64_t birth;
    };
    std::vector<TempCell> oldCells;
    if (keepColors && !aliveMap.empty() && oldCols > 0 && oldRows > 0) {
      for (int y = 0; y < oldRows; ++y) {
        size_t rowOff = (size_t)y * oldCols;
        for (int x = 0; x < oldCols; ++x) {
          size_t idx_l = rowOff + x;
          if (idx_l < aliveMap.size() && aliveMap[idx_l]) {
            oldCells.push_back({x, y, teamMap[idx_l], birthMap[idx_l]});
          }
        }
      }
    }
    int r = std::max(1, cfg.res);
    float maxShiftX = (totalW / 2.0f) / K_PARA;
    float maxShiftY = (totalH / 2.0f) / K_PARA;
    int padX = static_cast<int>(std::ceil(maxShiftX / r)) + 2;
    int padY = static_cast<int>(std::ceil(maxShiftY / r)) + 2;
    cachedPadX = static_cast<float>(padX);
    cachedPadY = static_cast<float>(padY);
    currentPadF = cachedPadX * static_cast<float>(r);
    int newCols = (totalW / r) + (static_cast<int>(cachedPadX) * 2);
    int newRows = (totalH / r) + (static_cast<int>(cachedPadY) * 2);
    size_t sz = static_cast<size_t>(newCols) * newRows;
    cols = newCols;
    rows = newRows;
    if (aliveMap.size() == sz) {
      std::fill(aliveMap.begin(), aliveMap.end(), 0);
      std::fill(teamMap.begin(), teamMap.end(), 0);
      std::fill(birthMap.begin(), birthMap.end(), 0);
      std::fill(cellGenList.begin(), cellGenList.end(), 0);
    } else {
      aliveMap.assign(sz, 0);
      teamMap.assign(sz, 0);
      birthMap.assign(sz, 0);
      cellGenList.assign(sz, 0);
    }
    fadeStates.assign(sz, {0, -1});
    spawnStates.assign(sz, {0, -1});
    activeCells.clear();
    nextActiveCells.clear();
    activeCells.reserve(sz);
    nextActiveCells.reserve(sz);
    for (int i = 0; i < static_cast<int>(sz); i++) activeCells.push_back(i);
    ensureTeamCapacity(4100);
    size_t cap = teamColors.size();
    teamVotes.resize(cap, 0);
    teamVotesGen.resize(cap, 0);
    teamPops.resize(cap, 0);
    teamLocs.resize(cap, -1);
    std::fill(teamActiveGen.begin(), teamActiveGen.end(), 0);
    for (auto& h : teamHistory) h.clear();
    std::fill(teamStasisCount.begin(), teamStasisCount.end(), 0);
    preX.assign(cols, 0.f);
    preY.assign(rows, 0.f);
    float rF = static_cast<float>(r);
    for (int x = 0; x < cols; x++) preX[x] = (static_cast<float>(x) - cachedPadX) * rF;
    for (int y = 0; y < rows; y++) preY[y] = (static_cast<float>(y) - cachedPadY) * rF;
    if (!keepColors) {
      tidCounter = 1;
      freeTeamIDs.clear();
      lastHue = std::uniform_real_distribution<float>(0.f, 1.f)(gen);
      currentBgHue = lastHue;
      targetBgHue = lastHue;
      currentRuntimeBgColor = fromHSV(currentBgHue, cfg.bgSaturation / 100.f, cfg.bgValue / 100.f);
      std::fill(teamColors.begin(), teamColors.end(), sf::Color::Black);
      ensureTeamCapacity(1);
    }
    currentGen = 1;
    if (keepColors && !oldCells.empty()) {
      int64_t now = globalClock.getElapsedTime().asMilliseconds();
      float ratio = static_cast<float>(oldRes) / static_cast<float>(r);
      nextActiveCells.clear();
      float oldCenterX = static_cast<float>(oldCols) * 0.5f;
      float oldCenterY = static_cast<float>(oldRows) * 0.5f;
      float newCenterX = static_cast<float>(cols) * 0.5f;
      float newCenterY = static_cast<float>(rows) * 0.5f;
      for (const auto& cell : oldCells) {
        float deltaX = static_cast<float>(cell.x) - oldCenterX;
        float deltaY = static_cast<float>(cell.y) - oldCenterY;
        int nx = static_cast<int>(std::round(newCenterX + deltaX * ratio));
        int ny = static_cast<int>(std::round(newCenterY + deltaY * ratio));
        int fillSize = (ratio > 1.0f) ? static_cast<int>(std::ceil(ratio)) : 1;
        for (int dy = 0; dy < fillSize; ++dy) {
          for (int dx = 0; dx < fillSize; ++dx) {
            int tx = nx + dx;
            int ty = ny + dy;
            if (tx >= 0 && tx < cols && ty >= 0 && ty < rows) {
              size_t nIdx = (size_t)ty * cols + tx;
              if (nIdx < aliveMap.size()) {
                aliveMap[nIdx] = 1;
                teamMap[nIdx] = cell.team;
                birthMap[nIdx] = (cell.birth > 0) ? cell.birth : now;
                markActive(static_cast<int>(nIdx));
              }
            }
          }
        }
      }
      activeCells = nextActiveCells;
    } else if (!forceReset && cfg.save_world && loadWorldState()) {
      activeCells = nextActiveCells;
    } else {
      int mX = cols / 5, mY = rows / 5;
      int safeX = std::clamp(mX + static_cast<int>(gen() % (cols - 2 * mX)), 0, cols - 1);
      int safeY = std::clamp(mY + static_cast<int>(gen() % (rows - 2 * mY)), 0, rows - 1);
      spawnCluster(safeX, safeY, 0);
      activeCells = nextActiveCells;
      if (tidCounter > 1 && !teamColors.empty()) {
        targetBgHue = getH(teamColors[1]);
        currentBgHue = targetBgHue;
        currentRuntimeBgColor = fromHSV(currentBgHue, cfg.bgSaturation / 100.f, cfg.bgValue / 100.f);
      }
    }
    unsigned int tW = static_cast<unsigned int>(totalW) + static_cast<unsigned int>(currentPadF * 2.f);
    unsigned int tH = static_cast<unsigned int>(totalH) + static_cast<unsigned int>(currentPadF * 2.f);
    if (mips.size() < 4) mips.resize(4);
    for (int i = 0; i < 4; i++) {
      unsigned int resW = (i == 0) ? tW : (i == 1) ? tW / 4
                                                   : tW / (4 * (1 << (i - 1)));
      unsigned int resH = (i == 0) ? tH : (i == 1) ? tH / 4
                                                   : tH / (4 * (1 << (i - 1)));
      (void)mips[i].resize({std::max(1u, resW), std::max(1u, resH)});
      mips[i].setSmooth(true);
      mips[i].display();
    }
    generateCellAtlas();
    simdParticles.clear();
    lastActiveVerticesCount = 0;
    activeAnimations = 100;
    needsResort = true;
    glowWeightsNeedUpdate = true;
    worldChanged = true;
    staticBufferNeedsUpdate = true;
  }
  void spawnCluster(int tx, int ty, int prnt) {
    std::uniform_real_distribution<float> d(0.f, 1.f);
    int64_t now = globalClock.getElapsedTime().asMilliseconds();
    int activeTeamsCount = 0;
    size_t limit = std::min(static_cast<size_t>(tidCounter), teamActiveGen.size());
    for (size_t t = 1; t < limit; t++) {
      if (teamActiveGen[t] >= currentGen - 1) {
        activeTeamsCount++;
      }
    }
    bool limitAllowsNewTeam = (cfg.max_teams == 0) || (activeTeamsCount < cfg.max_teams);
    float roll = d(gen);
    int divisor = std::max(1, activeTeamsCount);
    float chance = (cfg.mutation_chance / 100.f) / static_cast<float>(divisor);
    bool forceNew = (tidCounter == 1) || (limitAllowsNewTeam && (roll < chance));
    int id;
    if (forceNew) {
      if (!freeTeamIDs.empty()) {
        id = freeTeamIDs.back();
        freeTeamIDs.pop_back();
      } else {
        id = tidCounter++;
      }
      ensureTeamCapacity(id);
      for (int dy = -2; dy <= 2; dy++) {
        int ny = ty + dy;
        if (ny >= rows)
          ny -= rows;
        else if (ny < 0)
          ny += rows;
        size_t rowOffset = static_cast<size_t>(ny) * cols;
        for (int dx = -2; dx <= 2; dx++) {
          if (std::abs(dx) == 2 && std::abs(dy) == 2) continue;
          int nx = tx + dx;
          if (nx >= cols)
            nx -= cols;
          else if (nx < 0)
            nx += cols;
          size_t ix = rowOffset + nx;
          if (ix < aliveMap.size()) {
            if (aliveMap[ix]) {
              aliveMap[ix] = 0;
              teamMap[ix] = 0;
              markActive(static_cast<int>(ix));
            }
            if (ix < spawnStates.size()) spawnStates[ix].startTimeMs = -1;
          }
        }
      }
      std::vector<int> activeIDs;
      activeIDs.reserve(static_cast<size_t>(id));
      for (int t = 1; t < id; t++) {
        if (static_cast<size_t>(t) < teamActiveGen.size() && teamActiveGen[t] >= currentGen - 1) {
          activeIDs.push_back(t);
        }
      }
      if (id == 1)
        lastHue = fmod(lastHue + std::uniform_real_distribution<float>(-0.2f, 0.2f)(gen) + 1.0f, 1.0f);
      else
        lastHue = getBestUniqueHue(activeIDs);
      teamColors[id] = fromHSV(lastHue, std::uniform_real_distribution<float>(0.7f, 1.0f)(gen), std::uniform_real_distribution<float>(0.7f, 1.0f)(gen));
      sf::Color c = teamColors[id];
      colorCache[id] = {c, c.toInteger(), adj(c, cfg.convex_power), adj(c, -cfg.convex_power)};
    } else {
      id = (prnt > 0) ? prnt : (tidCounter - 1);
      if (id <= 0) id = 1;
      ensureTeamCapacity(id);
    }
    uint32_t randomBits = gen();
    int bitIndex = 0;
    for (int y = -2; y <= 2; y++) {
      int ny = ty + y;
      if (ny >= rows)
        ny -= rows;
      else if (ny < 0)
        ny += rows;
      size_t rowOffset = static_cast<size_t>(ny) * cols;
      for (int x = -2; x <= 2; x++) {
        if (std::abs(x) == 2 && std::abs(y) == 2) continue;
        if ((randomBits >> bitIndex) & 1) {
          int nx = tx + x;
          if (nx >= cols)
            nx -= cols;
          else if (nx < 0)
            nx += cols;
          size_t ix = rowOffset + nx;
          if (ix < aliveMap.size()) {
            aliveMap[ix] = 1;
            teamMap[ix] = id;
            birthMap[ix] = now;
            spawnStates[ix].teamId = id;
            spawnStates[ix].startTimeMs = now;
            markActive(static_cast<int>(ix));
          }
        }
        bitIndex++;
      }
    }
  }
  void update() {
    if (aliveMap.empty()) return;
    if (cfg.speed > 0 && logicClock.getElapsedTime().asMilliseconds() < cfg.speed) return;
    logicClock.restart();
    currentGen++;
    if (currentGen == 0) {
      std::fill(cellGenList.begin(), cellGenList.end(), 0);
      currentGen = 1;
    }
    if (teamHistory.size() < static_cast<size_t>(tidCounter)) {
      ensureTeamCapacity(tidCounter);
    }
    aL_buf.clear();
    changes.clear();
    ext.clear();
    size_t mapSize = aliveMap.size();
    changes.reserve(activeCells.size());
    aL_buf.reserve(32);
    int64_t now = globalClock.getElapsedTime().asMilliseconds();
    std::swap(ext, nextActiveCells);
    nextActiveCells.clear();
    nextActiveCells.reserve(ext.size() * 2);
    for (int i : ext) {
      if (i < 0 || static_cast<size_t>(i) >= mapSize) continue;
      int cy = i / cols;
      int cx = i - (cy * cols);
      int y_u = (cy == 0 ? rows - 1 : cy - 1);
      int y_d = (cy == rows - 1 ? 0 : cy + 1);
      int x_l = (cx == 0 ? cols - 1 : cx - 1);
      int x_r = (cx == cols - 1 ? 0 : cx + 1);
      size_t rU = static_cast<size_t>(y_u) * cols;
      size_t rS = static_cast<size_t>(cy) * cols;
      size_t rD = static_cast<size_t>(y_d) * cols;
      if ((aliveMap[i] || aliveMap[rU + x_l] || aliveMap[rU + cx] || aliveMap[rU + x_r] ||
           aliveMap[rS + x_l] || aliveMap[rS + x_r] ||
           aliveMap[rD + x_l] || aliveMap[rD + cx] || aliveMap[rD + x_r]) &&
          cellGenList[i] != currentGen) {
        cellGenList[i] = currentGen;
        nextActiveCells.push_back(i);
      }
    }
    size_t currentCapacity = teamColors.size();
    std::memset(teamVotes.data(), 0, currentCapacity * sizeof(int));
    std::memset(teamVotesGen.data(), 0, currentCapacity * sizeof(int));
    int f = std::clamp(cfg.aggression, 1, 8), bT = (f < 3) ? 3 : 2, cT = 9 - f;
    int protection = (f < 4) ? 2 : (f < 7) ? 1
                                           : 0;
    int aliveCount = 0, animCount = 0;
    for (int i : activeCells) {
      if (i < 0 || static_cast<size_t>(i) >= mapSize) continue;
      int cy = i / cols;
      int cx = i - (cy * cols);
      int y_u = (cy == 0 ? rows - 1 : cy - 1);
      int y_d = (cy == rows - 1 ? 0 : cy + 1);
      int x_l = (cx == 0 ? cols - 1 : cx - 1);
      int x_r = (cx == cols - 1 ? 0 : cx + 1);
      size_t rU = static_cast<size_t>(y_u) * cols;
      size_t rS = static_cast<size_t>(cy) * cols;
      size_t rD = static_cast<size_t>(y_d) * cols;
      const size_t nb[8] = {
          rU + x_l, rU + cx, rU + x_r,
          rS + x_l, rS + x_r,
          rD + x_l, rD + cx, rD + x_r};
      int n = 0, localT[8], localTCount = 0;
      const uint32_t currentVoteMarker = static_cast<uint32_t>(i) + 1;
      int8_t wasA = aliveMap[i];
      int32_t myTeam = teamMap[i];
      bool allSame = true;
      for (int k = 0; k < 8; k++) {
        size_t nI = nb[k];
        if (nI < mapSize && aliveMap[nI]) {
          n++;
          int tID = teamMap[nI];
          if (tID != myTeam) allSame = false;
          if (tID > 0 && static_cast<size_t>(tID) < teamVotes.size()) {
            if (teamVotesGen[tID] != currentVoteMarker) {
              teamVotesGen[tID] = currentVoteMarker;
              teamVotes[tID] = 0;
              if (localTCount < 8) {
                localT[localTCount++] = tID;
              }
            }
            teamVotes[tID]++;
          }
        } else {
          allSame = false;
        }
      }
      bool shouldL = wasA ? (n == 2 || n == 3) : (n == 3);
      int nextT = wasA ? myTeam : 0;
      if (wasA && n == 8 && allSame) {
        shouldL = true;
        nextT = myTeam;
      } else if (shouldL) {
        int dT = 0, mV = 0;
        for (int k = 0; k < localTCount; k++) {
          int tID = localT[k];
          if (teamVotes[tID] > mV) {
            mV = teamVotes[tID];
            dT = tID;
          }
        }
        if (!wasA) {
          nextT = (mV >= bT) ? dT : (localTCount == 0 ? 0 : localT[0]);
          spawnStates[i] = {nextT, now};
        } else if (dT != myTeam && dT != 0 && mV >= cT && mV > (teamVotes[myTeam] + protection)) {
          nextT = dT;
          spawnStates[i] = {nextT, now};
        }
      }
      if (shouldL) {
        aliveCount++;
        if (nextT > 0 && static_cast<size_t>(nextT) < teamActiveGen.size() && teamActiveGen[nextT] != currentGen) {
          teamActiveGen[nextT] = currentGen;
          aL_buf.push_back(nextT);
        }
      } else if (wasA) {
        fadeStates[i] = {myTeam, now};
      }
      if (shouldL || (shouldL != wasA)) {
        markActive(i);
      }
      if (shouldL != wasA || (shouldL && nextT != myTeam)) {
        changes.push_back({i, static_cast<int8_t>(shouldL), nextT});
      }
    }
    for (int i : activeCells) {
      if (i >= 0 && static_cast<size_t>(i) < mapSize) {
        bool isS = (spawnStates[i].startTimeMs != -1 && (now - spawnStates[i].startTimeMs) < (cfg.speed + 100));
        bool isF = (fadeStates[i].startTimeMs != -1 && (now - fadeStates[i].startTimeMs) < (cfg.fade_ms + 100));
        if (!isS) spawnStates[i].startTimeMs = -1;
        if (!isF) fadeStates[i].startTimeMs = -1;
        if (aliveMap[i] || isS || isF) animCount++;
      }
    }
    activeAnimations = animCount;
    for (const auto& ch : changes) {
      if (ch.idx >= 0 && static_cast<size_t>(ch.idx) < mapSize) {
        if (aliveMap[ch.idx] && !ch.alive) {
          if (cfg.dust && simdParticles.size() < MAX_PARTICLES) {
            uint32_t cellHash = (static_cast<uint32_t>(ch.idx) ^ (currentGen * 1103515245u)) * 1664525u + 1013904223u;
            if (((uint64_t)cellHash * 100 >> 32) == 0) {
              if (simdParticles.size() < MAX_PARTICLES) {
                int cy = ch.idx / cols;
                int cx = ch.idx - (cy * cols);
                sf::Vector2f startPos = {preX[cx] + cfg.res / 2.f, preY[cy] + cfg.res / 2.f};
                uint32_t pColorPacked = colorCache[teamMap[ch.idx]].packed;
                float phaseF = static_cast<float>((cellHash >> 24) & 0xFF) / 255.f * 6.283185f;
                float driftX = (static_cast<float>(cellHash & 0xFF) / 255.f) * 2.f - 1.f;
                float driftY = (static_cast<float>((cellHash >> 8) & 0xFF) / 255.f) * 2.f - 1.f;
                uint8_t pType = 0;
                uint32_t r = cellHash % 100u;
                if (r < 70)
                  pType = 0;
                else if (r < 90)
                  pType = 1;
                else
                  pType = 2;
                simdParticles.posX.push_back(startPos.x);
                simdParticles.posY.push_back(startPos.y);
                simdParticles.velX.push_back(driftX);
                simdParticles.velY.push_back(driftY);
                simdParticles.phase.push_back(phaseF);
                simdParticles.life.push_back(1.0f);
                simdParticles.color.push_back(pColorPacked);
                simdParticles.type.push_back(pType);
              }
            }
          }
        }
        if (ch.alive && !aliveMap[ch.idx]) birthMap[ch.idx] = now;
        aliveMap[ch.idx] = ch.alive;
        teamMap[ch.idx] = ch.team;
      }
    }
    if (!changes.empty()) {
      staticBufferNeedsUpdate = true;
    }
    if (aliveCount == 0 && nextActiveCells.empty() && animCount == 0) {
      std::error_code ec;
      std::filesystem::remove(get_cfg_path("world.dat"), ec);
      activeCells.clear();
      nextActiveCells.clear();
      aliveMap.clear();
      needRestart = true;
      return;
    }
    std::memset(teamPops.data(), 0, currentCapacity * sizeof(int));
    std::fill(teamLocs.begin(), teamLocs.end(), -1);
    for (int i : nextActiveCells) {
      if (i >= 0 && static_cast<size_t>(i) < mapSize && aliveMap[i]) {
        int id = teamMap[i];
        if (id >= 0 && static_cast<size_t>(id) < teamPops.size()) {
          teamPops[id]++;
          if (teamLocs[id] == -1 || (gen() % 10 == 0)) teamLocs[id] = i;
        }
      }
    }
    int maxP = 0, domT = -1;
    std::vector<int> teamsToRespawn;
    for (int id : aL_buf) {
      if (id <= 0 || static_cast<size_t>(id) >= teamPops.size() || static_cast<size_t>(id) >= teamHistory.size()) continue;
      int pop = teamPops[id];
      if (pop > maxP) {
        maxP = pop;
        domT = id;
      }
      auto& h = teamHistory[id];
      bool isRepeating = false;
      for (int pastPop : h) {
        if (pastPop == pop && pop > 0) {
          isRepeating = true;
          break;
        }
      }
      if (pop > 0 && isRepeating) {
        if (static_cast<size_t>(id) < teamStasisCount.size()) teamStasisCount[id]++;
      } else {
        if (static_cast<size_t>(id) < teamStasisCount.size()) teamStasisCount[id] = 0;
      }
      h.push_back(pop);
      if (h.size() > 10) h.pop_front();
      if (static_cast<size_t>(id) < teamStasisCount.size() && teamStasisCount[id] >= 25 && teamLocs[id] != -1) {
        teamsToRespawn.push_back(id);
      }
    }
    bool spawnedAny = false;
    for (int id : teamsToRespawn) {
      int cellIdx = teamLocs[id];
      if (cellIdx != -1) {
        int s_cy = cellIdx / cols;
        int s_cx = cellIdx - (s_cy * cols);
        spawnCluster(s_cx, s_cy, id);
        spawnedAny = true;
      }
    }
    if (spawnedAny) {
      if (teamHistory.size() < static_cast<size_t>(tidCounter)) {
        ensureTeamCapacity(tidCounter);
      }
      size_t updatedCapacity = teamColors.size();
      std::memset(teamPops.data(), 0, updatedCapacity * sizeof(int));
      std::fill(teamLocs.begin(), teamLocs.end(), -1);
      for (int id : teamsToRespawn) {
        teamStasisCount[id] = 0;
        teamHistory[id].clear();
      }
    }
    if (domT != -1 && maxP > 20 && static_cast<size_t>(domT) < teamColors.size()) {
      targetBgHue = getH(teamColors[domT]);
    }
    if (currentGen % 512 == 0) collectGarbageTeams();
    std::swap(activeCells, nextActiveCells);
    nextActiveCells.clear();
    needsResort = true;
    worldChanged = true;
    ext.clear();
    for (const auto& ch : changes) {
      ext.push_back(ch.idx);
    }
    for (const auto& item : localAnim) {
      ext.push_back(item.idx);
    }
    std::sort(ext.begin(), ext.end());
    ext.erase(std::unique(ext.begin(), ext.end()), ext.end());
    std::sort(ext.begin(), ext.end(), [this](int a, int b) {
      return birthMap[a] < birthMap[b];
    });
    localAnim.clear();
    for (int index : ext) {
      if (index < 0 || static_cast<size_t>(index) >= mapSize) continue;
      bool isS = (spawnStates[index].startTimeMs != -1 && (now - spawnStates[index].startTimeMs) < (cfg.speed + 100));
      bool isF = (fadeStates[index].startTimeMs != -1 && (now - fadeStates[index].startTimeMs) < (cfg.fade_ms + 100));
      if (isS || isF) {
        int cy = index / cols;
        int cx = index - (cy * cols);
        localAnim.push_back({cx, cy, teamMap[index], index});
      }
    }
    staticBufferNeedsUpdate = true;
  }
  void appendBox(std::vector<sf::Vertex>& va, size_t& vPtr, int x, int y, float sc, float fBSize, const PrecalculatedUV& uv, sf::Color c) {
    static constexpr size_t VERTICES_PER_BOX = 6;
    if (vPtr + VERTICES_PER_BOX > va.size()) return;
    if (x < 0 || x >= cols || y < 0 || y >= rows) return;
    float baseRes = static_cast<float>(cfg.res);
    float finalSize = (fBSize + 2.0f) * sc;
    if (finalSize < 1.0f) finalSize = 1.0f;
    float offsetCenter = (baseRes - finalSize) * 0.5f;
    float vx = preX[x] + offsetCenter + 2.0f;
    float vy = preY[y] + offsetCenter + 2.0f;
    sf::Vertex* v = &va[vPtr];
    v[0].position = {vx, vy};
    v[0].texCoords = {uv.tx0, uv.ty0};
    v[0].color = c;
    v[1].position = {vx + finalSize, vy};
    v[1].texCoords = {uv.tx1, uv.ty0};
    v[1].color = c;
    v[2].position = {vx + finalSize, vy + finalSize};
    v[2].texCoords = {uv.tx1, uv.ty1};
    v[2].color = c;
    v[3].position = {vx, vy};
    v[3].texCoords = {uv.tx0, uv.ty0};
    v[3].color = c;
    v[4].position = {vx + finalSize, vy + finalSize};
    v[4].texCoords = {uv.tx1, uv.ty1};
    v[4].color = c;
    v[5].position = {vx, vy + finalSize};
    v[5].texCoords = {uv.tx0, uv.ty1};
    v[5].color = c;
    vPtr += VERTICES_PER_BOX;
  }
  void appendStaticBox(std::vector<sf::Vertex>& va, size_t& vPtr, int x, int y, const PrecalculatedUV& uv, sf::Color c, float finalSize, float offsetCenter) {
    if (vPtr + 6 > va.size()) return;
    float vx = preX[x] + offsetCenter;
    float vy = preY[y] + offsetCenter;
    sf::Vertex* v = &va[vPtr];
    v[0].position = {vx, vy};
    v[0].texCoords = {uv.tx0, uv.ty0};
    v[0].color = c;
    v[1].position = {vx + finalSize, vy};
    v[1].texCoords = {uv.tx1, uv.ty0};
    v[1].color = c;
    v[2].position = {vx + finalSize, vy + finalSize};
    v[2].texCoords = {uv.tx1, uv.ty1};
    v[2].color = c;
    v[3].position = {vx, vy};
    v[3].texCoords = {uv.tx0, uv.ty0};
    v[3].color = c;
    v[4].position = {vx + finalSize, vy + finalSize};
    v[4].texCoords = {uv.tx1, uv.ty1};
    v[4].color = c;
    v[5].position = {vx, vy + finalSize};
    v[5].texCoords = {uv.tx0, uv.ty1};
    v[5].color = c;
    vPtr += 6;
  }
  void render(sf::RenderTarget& window, sf::RenderWindow& nativeWindow) {
    if (cellAtlasRT.getTexture().getSize().x == 0) {
      generateCellAtlas();
    }
    static sf::Clock pClock;
    static int bloomTick = 0;
    static size_t totalNeeded = 0;
    if (worldChanged) {
      totalNeeded = 0;
      activeAnimations = 0;
    }
    float pF = currentPadF;
    float dt = pClock.restart().asSeconds();
    float gD = 1.0f;
    float ts = static_cast<float>(cfg.res) + 4.0f;
    float ty0 = 0.5f;
    float ty1 = ts - 0.5f;
    PrecalculatedUV uvMask;
    uvMask.tx0 = 0.5f * (ts * 2.0f) + 0.5f;
    uvMask.tx1 = uvMask.tx0 + ts - 0.5f;
    uvMask.ty0 = ty0;
    uvMask.ty1 = ty1;
    PrecalculatedUV uvCells;
    uvCells.tx0 = 0.0f * (ts * 2.0f) + 0.5f;
    uvCells.tx1 = uvCells.tx0 + ts - 0.5f;
    uvCells.ty0 = ty0;
    uvCells.ty1 = ty1;
    if (cfg.parallax_enabled && !isPaused.load() && !autoPaused.load()) {
      sf::Vector2i m = sf::Mouse::getPosition(nativeWindow);
      targetParallax = {(static_cast<float>(m.x) - totalW / 2.f) / K_PARA, (static_cast<float>(m.y) - totalH / 2.f) / K_PARA};
      parallaxOffset += (targetParallax - parallaxOffset) * (dt * 5.0f);
    } else {
      targetParallax = parallaxOffset;
    }
    sf::Vector2f snappedOffset{std::round(parallaxOffset.x), std::round(parallaxOffset.y)};
    float hueDiff = targetBgHue - currentBgHue;
    if (std::abs(hueDiff) > 0.001f) {
      if (hueDiff > 0.5f) hueDiff -= 1.0f;
      if (hueDiff < -0.5f) hueDiff += 1.0f;
      currentBgHue += hueDiff * (dt * 0.05f);
      if (currentBgHue < 0.f) currentBgHue += 1.f;
      if (currentBgHue > 1.f) currentBgHue -= 1.f;
      currentRuntimeBgColor = fromHSV(currentBgHue, cfg.bgSaturation / 100.f, cfg.bgValue / 100.f);
    }
    if (!(isPaused.load() || autoPaused.load())) {
      internalTime += dt;
      if (internalTime >= 628.3185307f) {
        internalTime = std::fmod(internalTime, 628.3185307f);
      }
    }
    float sT = internalTime;
    float rayDim = 1.0f;
    rt2.clear(sf::Color(0, 0, 0, 0));
    sf::View bgView;
    bgView.setCenter({totalW / 2.f, totalH / 2.f});
    bgView.setSize({static_cast<float>(totalW), static_cast<float>(totalH)});
    rt2.setView(bgView);
    sf::Vector2f rt2Res = static_cast<sf::Vector2f>(rt2.getTexture().getSize());
    raysShader.setUniform("u_resolution", rt2Res);
    sf::Glsl::Vec4 timesVector(sT * 0.01f, sT * 0.3f, sT * 0.5f, sT * 0.8f);
    raysShader.setUniform("u_times", timesVector);
    raysShader.setUniform("color", sf::Glsl::Vec3({(currentRuntimeBgColor.r / 255.f) * rayDim, (currentRuntimeBgColor.g / 255.f) * rayDim, (currentRuntimeBgColor.b / 255.f) * rayDim}));
    rt2.draw(raysRect, &raysShader);
    rt2.display();
    if (colorUpdateClock.getElapsedTime().asSeconds() > 2.0f) {
      updateColorCache(gD);
      colorUpdateClock.restart();
    }
    float baseRes = static_cast<float>(cfg.res);
    float fBSize = baseRes - static_cast<float>(cfg.gap);
    float staticFinalSize = fBSize + 2.0f;
    if (staticFinalSize < 1.0f) staticFinalSize = 1.0f;
    float staticOffsetCenter = ((baseRes - staticFinalSize) * 0.5f) + 2.0f;
    float cSz = static_cast<float>(cfg.res);
    int padC = static_cast<int>(std::ceil(pF / cSz)) + 1;
    int x0 = std::max(0, static_cast<int>(snappedOffset.x / cSz) - padC);
    int x1 = std::min(cols, static_cast<int>((snappedOffset.x + totalW) / cSz) + padC + 1);
    int y0 = std::max(0, static_cast<int>(snappedOffset.y / cSz) - padC);
    int y1 = std::min(rows, static_cast<int>((snappedOffset.y + totalH) / cSz) + padC + 1);
    static size_t staticTotalVertices = 0;
    if (staticBufferNeedsUpdate) {
      static std::vector<int> visibleIndices;
      visibleIndices.clear();
      for (int y = y0; y < y1; y++) {
        size_t rOff = static_cast<size_t>(y) * cols;
        for (int x = x0; x < x1; x++) {
          size_t i = rOff + x;
          if (i < aliveMap.size() && spawnStates[i].startTimeMs == -1 && fadeStates[i].startTimeMs == -1 && aliveMap[i]) {
            visibleIndices.push_back(static_cast<int>(i));
          }
        }
      }
      size_t visibleStaticCount = visibleIndices.size();
      std::sort(visibleIndices.begin(), visibleIndices.end(), [this](int a, int b) {
        return birthMap[a] < birthMap[b];
      });
      size_t staticTotal = visibleStaticCount * 12;
      if (staticVB.getVertexCount() < staticTotal) {
        size_t safeMaxStatic = static_cast<size_t>(cols * rows * 12);
        if (staticVB.getVertexCount() < safeMaxStatic) {
          if (staticVB.create(safeMaxStatic)) {
            staticVB.setPrimitiveType(sf::PrimitiveType::Triangles);
            staticVB.setUsage(sf::VertexBuffer::Usage::Static);
          }
          staticVertexCPU.resize(safeMaxStatic);
        }
      }
      size_t sPtrMask = 0;
      size_t sPtrCells = visibleStaticCount * 6;
      for (int i : visibleIndices) {
        int cx = i % cols;
        int cy = i / cols;
        int tid = teamMap[i];
        if (tid > 0 && static_cast<size_t>(tid) < colorCache.size()) {
          sf::Color c = colorCache[tid].main;
          sf::Color cCells = c;
          cCells.a = static_cast<uint8_t>(gD * 255.f);
          sf::Color cMask = c;
          cMask.a = static_cast<uint8_t>(gD * 0.7f * 255.f);
          appendStaticBox(staticVertexCPU, sPtrMask, cx, cy, uvMask, cMask, staticFinalSize, staticOffsetCenter);
          appendStaticBox(staticVertexCPU, sPtrCells, cx, cy, uvCells, cCells, staticFinalSize, staticOffsetCenter);
        }
      }
      staticTotalVertices = sPtrCells;
      if (staticTotalVertices > 0) {
        if (!staticVB.update(staticVertexCPU.data(), static_cast<unsigned int>(staticTotalVertices), 0)) {
          sf::err() << "Warning: staticVB update failed" << std::endl;
        }
      }
      staticBufferNeedsUpdate = false;
      needsResort = false;
    }
    size_t dynTotal = localAnim.size() * 12;
    if (mainVB.getVertexCount() < dynTotal) {
      size_t safeMaxDynamic = static_cast<size_t>(cols * rows * 12);
      if (mainVB.getVertexCount() < safeMaxDynamic) {
        if (mainVB.create(safeMaxDynamic)) {
          mainVB.setPrimitiveType(sf::PrimitiveType::Triangles);
          mainVB.setUsage(sf::VertexBuffer::Usage::Stream);
        }
        vertexCPU.resize(safeMaxDynamic);
      }
    }
    if (lastActiveVerticesCount > 0 && lastActiveVerticesCount <= vertexCPU.size()) {
      std::fill(vertexCPU.begin(), vertexCPU.begin() + lastActiveVerticesCount, sf::Vertex{});
    }
    size_t vPtrMask = 0;
    size_t vP_cells_start = localAnim.size() * 6;
    size_t vPtrCells = vP_cells_start;
    int64_t now = globalClock.getElapsedTime().asMilliseconds();
    for (const auto& item : localAnim) {
      if (item.id <= 0 || static_cast<size_t>(item.id) >= colorCache.size()) continue;
      float current_fade_ms = static_cast<float>(cfg.fade_ms);
      float d = (current_fade_ms > 1.f) ? static_cast<float>((static_cast<uint32_t>(item.idx) * 1337u) % 100u) / 100.f * (current_fade_ms * 0.7f) : 0.f;
      float sc = 1.0f;
      int tid = item.id;
      bool isF = false;
      if (fadeStates[item.idx].startTimeMs != -1) {
        if (current_fade_ms <= 5.f) {
          sc = 0.0f;
        } else {
          float el = static_cast<float>(now - fadeStates[item.idx].startTimeMs) - d;
          sc = (el < 0) ? 1.0f : std::max(0.f, 1.0f - std::clamp(el / current_fade_ms, 0.f, 1.f));
        }
        tid = fadeStates[item.idx].teamId;
        isF = true;
      } else if (spawnStates[item.idx].startTimeMs != -1) {
        float el = static_cast<float>(now - spawnStates[item.idx].startTimeMs) - d;
        float speedFactor = cfg.speed * 0.9f;
        sc = (el < 0) ? 0.f : (speedFactor > 0.f ? std::clamp(el / speedFactor, 0.f, 1.f) : 1.f);
        tid = spawnStates[item.idx].teamId;
      }
      if (sc > 0.01f) {
        if (tid > 0 && static_cast<size_t>(tid) < colorCache.size()) {
          sf::Color c = colorCache[tid].main;
          float alphaFactor = isF ? (cfg.alpha_fade ? (gD * sc) : gD) : gD;
          sf::Color cCells = c;
          cCells.a = static_cast<uint8_t>(alphaFactor * 255.f);
          sf::Color cMask = c;
          cMask.a = static_cast<uint8_t>(alphaFactor * 0.7f * 255.f);
          appendBox(vertexCPU, vPtrMask, item.x, item.y, sc, fBSize, uvMask, cMask);
          appendBox(vertexCPU, vPtrCells, item.x, item.y, sc, fBSize, uvCells, cCells);
        }
      }
    }
    if (vPtrCells > 0 && vPtrCells <= vertexCPU.size()) {
      if (!mainVB.update(vertexCPU.data(), static_cast<unsigned int>(vPtrCells), 0)) {
        sf::err() << "Warning: mainVB update failed" << std::endl;
      }
    }
    size_t totalNeededCount = vPtrCells;
    lastActiveVerticesCount = vPtrCells;
    sf::View staticView;
    staticView.setSize({static_cast<float>(totalW) + (pF * 2.f), static_cast<float>(totalH) + (pF * 2.f)});
    staticView.setCenter({(totalW / 2.f), (totalH / 2.f)});
    mips[0].clear(sf::Color::Transparent);
    mips[0].setView(staticView);
    fastCellShader.setUniform("isMask", 1.0f);
    sf::RenderStates rsGlow;
    rsGlow.shader = &fastCellShader;
    rsGlow.texture = &cellAtlasRT.getTexture();
    rsGlow.blendMode = sf::BlendAdd;
    if (vP_cells_start > 0) {
      mips[0].draw(mainVB, 0, static_cast<unsigned int>(vP_cells_start), rsGlow);
    }
    if (staticTotalVertices > 0) {
      mips[0].draw(staticVB, 0, static_cast<unsigned int>(staticTotalVertices / 2), rsGlow);
    }
    if (!cfg.only_glow) {
      fastCellShader.setUniform("isMask", 0.0f);
      sf::RenderStates rsMain;
      rsMain.shader = &fastCellShader;
      rsMain.texture = &cellAtlasRT.getTexture();
      rsMain.blendMode = sf::BlendMode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::OneMinusSrcAlpha);
      if (vPtrCells > vP_cells_start) {
        mips[0].draw(mainVB, static_cast<unsigned int>(vP_cells_start), static_cast<unsigned int>(vPtrCells - vP_cells_start), rsMain);
      }
      if (staticTotalVertices > 0) {
        size_t sCellsStart = staticTotalVertices / 2;
        mips[0].draw(staticVB, static_cast<unsigned int>(sCellsStart), static_cast<unsigned int>(staticTotalVertices / 2), rsMain);
      }
    }
    if (!mips.empty()) {
      mips[0].display();
    }
    if (!isPaused && cfg.glow_sigma >= 0.5f) {
      bloomOffset = snappedOffset;
      float finalGlowValue = (cfg.glow_sigma * 0.1f) / 2.0f * gD;
      shUp.setUniform("glow", finalGlowValue);
      shUpChrom.setUniform("glow", finalGlowValue);
      shUpChrom.setUniform("chromAb", 1.0f);
      const size_t mipsCount = mips.size();
      for (size_t i = 0; i < 3; i++) {
        if (i + 1 >= mipsCount) break;
        sf::Vector2f sizeF = static_cast<sf::Vector2f>(mips[i + 1].getSize());
        mips[i + 1].setView(sf::View(sf::FloatRect({0.f, 0.f}, sizeF)));
        shDown.setUniform("u_res", sizeF);
        sf::Sprite spDown(mips[i].getTexture());
        sf::Vector2f currentSize = static_cast<sf::Vector2f>(mips[i].getTexture().getSize());
        spDown.setScale({sizeF.x / currentSize.x, sizeF.y / currentSize.y});
        sf::RenderStates states;
        states.blendMode = sf::BlendNone;
        states.shader = &shDown;
        mips[i + 1].draw(spDown, states);
        mips[i + 1].display();
      }
      for (size_t i = mipsCount - 1; i > 1; i--) {
        if (i >= mipsCount || i - 1 < 0) continue;
        sf::Vector2f sizeF = static_cast<sf::Vector2f>(mips[i - 1].getSize());
        mips[i - 1].setView(sf::View(sf::FloatRect({0.f, 0.f}, sizeF)));
        sf::Shader* activeUpShader = (cfg.chromatic_aberration && i == 2) ? &shUpChrom : &shUp;
        activeUpShader->setUniform("u_res", sizeF);
        sf::Sprite spUp(mips[i].getTexture());
        spUp.setScale({4.0f, 4.0f});
        sf::RenderStates states;
        states.blendMode = sf::BlendNone;
        states.shader = activeUpShader;
        mips[i - 1].draw(spUp, states);
        mips[i - 1].display();
      }
    }
    sf::View dustView;
    dustView.setSize({static_cast<float>(totalW) + (pF * 2.f), static_cast<float>(totalH) + (pF * 2.f)});
    dustView.setCenter({(totalW / 2.f), (totalH / 2.f)});
    window.clear(currentRuntimeBgColor);
    window.setView(window.getDefaultView());
    sf::Sprite raysSp(rt2.getTexture());
    raysSp.setScale({4.f, 4.f});
    window.draw(raysSp, sf::BlendAdd);
    if (cfg.glow_sigma >= 0.5f && mips.size() > 1) {
      sf::Sprite glowSp(mips[1].getTexture());
      glowSp.setScale({4.f, 4.f});
      sf::Vector2f correction = snappedOffset - bloomOffset;
      glowSp.setPosition({std::round(-pF - snappedOffset.x + correction.x), std::round(-pF - snappedOffset.y + correction.y)});
      window.draw(glowSp, sf::BlendAdd);
    }
    if (cfg.only_glow) {
      mips[0].clear(sf::Color::Transparent);
    }
    if (cfg.dust && !simdParticles.empty()) {
      size_t vPtr = 0;
      float dynamicMargin = currentPadF;
      float boundW = static_cast<float>(totalW);
      float boundH = static_cast<float>(totalH);
      static const float LUT_pShift[] = {-0.2f, 0.2f, 0.8f};
      static const float LUT_sizeVal[] = {1.0f, 2.0f, 3.0f};
      static const float LUT_baseAlpha[] = {250.f, 190.f, 110.f};
      float sliderValue = static_cast<float>(500 - cfg.speed);
      float lifeInSeconds = std::max(30.0f, 500.0f - sliderValue) * 2.0f;
      float lifeStep = dt / lifeInSeconds;
      float dtMult5 = dt * 5.0f;
      bool updatePhysics = !(isPaused.load() || autoPaused.load());
      size_t pSize = simdParticles.size();
      if (updatePhysics && pSize >= 4) {
        __m128 mDt = _mm_set1_ps(dt);
        __m128 mDtMult5 = _mm_set1_ps(dtMult5);
        __m128 mLifeStep = _mm_set1_ps(lifeStep);
        __m128 mMaxW = _mm_set1_ps(boundW + dynamicMargin);
        __m128 mMinW = _mm_set1_ps(-dynamicMargin);
        __m128 mMaxH = _mm_set1_ps(boundH + dynamicMargin);
        __m128 mMinH = _mm_set1_ps(-dynamicMargin);
        size_t simdLimit = pSize - (pSize % 4);
        for (size_t i = 0; i < simdLimit; i += 4) {
          __m128 pX = _mm_loadu_ps(&simdParticles.posX[i]);
          __m128 pY = _mm_loadu_ps(&simdParticles.posY[i]);
          __m128 vX = _mm_loadu_ps(&simdParticles.velX[i]);
          __m128 vY = _mm_loadu_ps(&simdParticles.velY[i]);
          __m128 ph = _mm_loadu_ps(&simdParticles.phase[i]);
          __m128 lf = _mm_loadu_ps(&simdParticles.life[i]);
          ph = _mm_add_ps(ph, mDt);
          pX = _mm_add_ps(pX, _mm_mul_ps(vX, mDtMult5));
          pY = _mm_add_ps(pY, _mm_mul_ps(vY, mDtMult5));
          lf = _mm_sub_ps(lf, mLifeStep);
          __m128 maskMaxX = _mm_cmpgt_ps(pX, mMaxW);
          __m128 maskMinX = _mm_cmplt_ps(pX, mMinW);
          pX = _mm_or_ps(_mm_andnot_ps(maskMaxX, pX), _mm_and_ps(maskMaxX, mMinW));
          pX = _mm_or_ps(_mm_andnot_ps(maskMinX, pX), _mm_and_ps(maskMinX, mMaxW));
          __m128 maskMaxY = _mm_cmpgt_ps(pY, mMaxH);
          __m128 maskMinY = _mm_cmplt_ps(pY, mMinH);
          pY = _mm_or_ps(_mm_andnot_ps(maskMaxY, pY), _mm_and_ps(maskMaxY, mMinH));
          pY = _mm_or_ps(_mm_andnot_ps(maskMinY, pY), _mm_and_ps(maskMinY, mMaxH));
          _mm_storeu_ps(&simdParticles.posX[i], pX);
          _mm_storeu_ps(&simdParticles.posY[i], pY);
          _mm_storeu_ps(&simdParticles.phase[i], ph);
          _mm_storeu_ps(&simdParticles.life[i], lf);
        }
      }
      for (size_t i = 0; i < pSize;) {
        if (updatePhysics && i >= (pSize - (pSize % 4))) {
          simdParticles.phase[i] += dt;
          simdParticles.posX[i] += simdParticles.velX[i] * dtMult5;
          simdParticles.posY[i] += simdParticles.velY[i] * dtMult5;
          simdParticles.life[i] -= lifeStep;
          if (simdParticles.posX[i] > boundW + dynamicMargin)
            simdParticles.posX[i] = -dynamicMargin;
          else if (simdParticles.posX[i] < -dynamicMargin)
            simdParticles.posX[i] = boundW + dynamicMargin;
          if (simdParticles.posY[i] > boundH + dynamicMargin)
            simdParticles.posY[i] = -dynamicMargin;
          else if (simdParticles.posY[i] < -dynamicMargin)
            simdParticles.posY[i] = boundH + dynamicMargin;
        }
        if (simdParticles.life[i] <= 0.f) {
          if (i < pSize - 1) {
            simdParticles.posX[i] = simdParticles.posX.back();
            simdParticles.posY[i] = simdParticles.posY.back();
            simdParticles.velX[i] = simdParticles.velX.back();
            simdParticles.velY[i] = simdParticles.velY.back();
            simdParticles.phase[i] = simdParticles.phase.back();
            simdParticles.life[i] = simdParticles.life.back();
            simdParticles.color[i] = simdParticles.color.back();
            simdParticles.type[i] = simdParticles.type.back();
          }
          simdParticles.pop_back();
          pSize--;
          continue;
        }
        i++;
      }
      size_t maxVertices = particlesCPU.size();
      size_t maxFillCount = std::min(pSize, maxVertices / 6);
      sf::Vertex* rawVerticesPtr = particlesCPU.data();
      for (size_t i = 0; i < maxFillCount; ++i) {
        int safeType = static_cast<int>(simdParticles.type[i]);
        if (safeType < 0 || safeType > 2) safeType = 0;
        float currentShift = LUT_pShift[safeType];
        float sVal = LUT_sizeVal[safeType];
        float baseAlpha = LUT_baseAlpha[safeType];
        float pX = simdParticles.posX[i] + fastSin(simdParticles.phase[i]) * (15.0f * currentShift);
        float pY = simdParticles.posY[i] + fastCos(simdParticles.phase[i] * 0.7f) * (10.0f * currentShift);
        sf::Color c;
        uint32_t packedColor = simdParticles.color[i];
        c.r = static_cast<uint8_t>((packedColor >> 24) & 0xFF);
        c.g = static_cast<uint8_t>((packedColor >> 16) & 0xFF);
        c.b = static_cast<uint8_t>((packedColor >> 8) & 0xFF);
        c.a = static_cast<uint8_t>(baseAlpha * gD * simdParticles.life[i]);
        float px = pX - (parallaxOffset.x * currentShift);
        float py = pY - (parallaxOffset.y * currentShift);
        sf::Vector2f txCoord = {simdParticles.life[i], 0.0f};
        sf::Vertex* v = &rawVerticesPtr[vPtr];
        v[0].position = {px, py};
        v[0].texCoords = txCoord;
        v[0].color = c;
        v[1].position = {px + sVal, py};
        v[1].texCoords = txCoord;
        v[1].color = c;
        v[2].position = {px + sVal, py + sVal};
        v[2].texCoords = txCoord;
        v[2].color = c;
        v[3].position = {px, py};
        v[3].texCoords = txCoord;
        v[3].color = c;
        v[4].position = {px + sVal, py + sVal};
        v[4].texCoords = txCoord;
        v[4].color = c;
        v[5].position = {px, py + sVal};
        v[5].texCoords = txCoord;
        v[5].color = c;
        vPtr += 6;
      }
      if (vPtr > 0 && !mips.empty()) {
        mips[0].setView(dustView);
        sf::RenderStates pStates;
        pStates.blendMode = sf::BlendAdd;
        fastCellShader.setUniform("isMask", 0.0f);
        pStates.shader = &fastCellShader;
        pStates.texture = nullptr;
        mips[0].draw(rawVerticesPtr, vPtr, sf::PrimitiveType::Triangles, pStates);
        mips[0].display();
      }
    } else if (!cfg.dust && !simdParticles.empty()) {
      simdParticles.clear();
      if (!mips.empty()) mips[0].display();
    }
    if (!mips.empty()) {
      sf::Sprite cellsSp(mips[0].getTexture());
      cellsSp.setPosition({std::round(-pF - snappedOffset.x), std::round(-pF - snappedOffset.y)});
      window.draw(cellsSp, sf::BlendMode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::OneMinusSrcAlpha));
    }
    if (vLoaded) {
      sf::Vector2f winRes = static_cast<sf::Vector2f>(window.getSize());
      vignetteShader.setUniform("u_resolution", winRes);
      window.draw(vigRect, sf::RenderStates(&vignetteShader));
    }
  }
  void loadConfig() {
    std::ifstream f(get_cfg_path("settings.cfg"));
    std::locale classicLoc = std::locale::classic();
    Config defaultCfg;
    if (f.is_open()) {
      std::string line, k, v;
      while (std::getline(f, line)) {
        std::stringstream ss(line);
        ss.imbue(classicLoc);
        if (ss >> k) {
          if (k == "LANG") {
            ss >> v;
            isRussian = (v == "1");
          } else if (k == "RES") {
            ss >> cfg.res;
          } else if (k == "GAP") {
            ss >> cfg.gap;
          } else if (k == "ROUND") {
            ss >> cfg.round;
          } else if (k == "SPEED") {
            ss >> cfg.speed;
          } else if (k == "CONVEX") {
            ss >> cfg.convex_power;
          } else if (k == "BEVEL") {
            ss >> cfg.bevel_ratio;
          } else if (k == "FADE") {
            ss >> cfg.fade_ms;
          } else if (k == "MUTATION") {
            ss >> cfg.mutation_chance;
          } else if (k == "MAXTEAMS") {
            ss >> cfg.max_teams;
          } else if (k == "AGGRESSION") {
            ss >> cfg.aggression;
          } else if (k == "FPS") {
            ss >> cfg.fps;
          } else if (k == "GLOW") {
            ss >> cfg.glow_sigma;
          } else if (k == "DUST") {
            ss >> v;
            cfg.dust = (v == "1");
          } else if (k == "AL_FADE") {
            ss >> v;
            cfg.alpha_fade = (v == "1");
          } else if (k == "MOUSE") {
            ss >> v;
            cfg.mouse_enabled = (v == "1");
          } else if (k == "ONLY_GLOW") {
            ss >> v;
            cfg.only_glow = (v == "1");
          } else if (k == "AUTO_PAUSE") {
            ss >> v;
            cfg.auto_pause = (v == "1");
          } else if (k == "CHROM_AB") {
            ss >> v;
            cfg.chromatic_aberration = (v == "1");
          } else if (k == "SAVE_WORLD") {
            ss >> v;
            cfg.save_world = (v == "1");
          } else if (k == "PARALLAX") {
            ss >> v;
            cfg.parallax_enabled = (v == "1");
          } else if (k == "BG_SAT") {
            ss >> cfg.bgSaturation;
          } else if (k == "BG_VAL") {
            ss >> cfg.bgValue;
          }
        }
      }
      f.close();
    }
    std::string s;
    cfg.res = std::clamp(cfg.res, 4, 100);
    cfg.gap = std::clamp(cfg.gap, -50, 50);
    cfg.round = std::clamp(cfg.round, 0, 100);
    cfg.speed = std::clamp(cfg.speed, 0, 500);
    cfg.convex_power = std::clamp(cfg.convex_power, 0, 150);
    cfg.bevel_ratio = std::clamp(cfg.bevel_ratio, 0.0f, 1.0f);
    cfg.fade_ms = std::clamp(cfg.fade_ms, 0, 5000);
    cfg.mutation_chance = std::clamp(cfg.mutation_chance, 0.0f, 50.0f);
    cfg.max_teams = std::clamp(cfg.max_teams, 0, 20);
    cfg.aggression = std::clamp(cfg.aggression, 1, 8);
    cfg.fps = std::clamp(cfg.fps, 15, 144);
    cfg.glow_sigma = std::clamp(cfg.glow_sigma, 0.0f, 50.0f);
    cfg.bgSaturation = std::clamp(cfg.bgSaturation, 0.0f, 100.0f);
    cfg.bgValue = std::clamp(cfg.bgValue, 0.0f, 100.0f);
    glowWeightsNeedUpdate = true;
    updateColorCache(getCurrentDim());
    generateCellAtlas();
  }
  void saveCurrentConfig() {
    std::string finalPath = get_cfg_path("settings.cfg");
    std::string tmpPath = finalPath + ".tmp";
    std::ofstream o(tmpPath);
    if (!o.is_open()) return;
    o.imbue(std::locale::classic());
    o << std::fixed << std::setprecision(4)
      << "LANG " << (isRussian ? 1 : 0)
      << "\nRES " << cfg.res
      << "\nGAP " << cfg.gap
      << "\nROUND " << cfg.round
      << "\nSPEED " << cfg.speed
      << "\nCONVEX " << cfg.convex_power
      << "\nBEVEL " << cfg.bevel_ratio
      << "\nFADE " << cfg.fade_ms
      << "\nMUTATION " << cfg.mutation_chance
      << "\nMAXTEAMS " << cfg.max_teams
      << "\nAGGRESSION " << cfg.aggression
      << "\nFPS " << cfg.fps
      << "\nGLOW " << cfg.glow_sigma
      << "\nCHROM_AB " << (cfg.chromatic_aberration ? 1 : 0)
      << "\nDUST " << (cfg.dust ? 1 : 0)
      << "\nAL_FADE " << (cfg.alpha_fade ? 1 : 0)
      << "\nMOUSE " << (cfg.mouse_enabled ? 1 : 0)
      << "\nONLY_GLOW " << (cfg.only_glow ? 1 : 0)
      << "\nAUTO_PAUSE " << (cfg.auto_pause ? 1 : 0)
      << "\nSAVE_WORLD " << (cfg.save_world ? 1 : 0)
      << "\nPARALLAX " << (cfg.parallax_enabled ? 1 : 0)
      << "\nBG_SAT " << cfg.bgSaturation
      << "\nBG_VAL " << cfg.bgValue << "\n";
    o.close();
    std::error_code ec;
    fs::rename(tmpPath, finalPath, ec);
  }
  bool getIsActivated() {
    return true;
  }
  Config& getCfg() { return cfg; }
  int getCols() { return cols; }
  int getRows() { return rows; }
};
SyncedLifeEngine* globalEng = nullptr;
void renderSettingsWindow(sf::RenderWindow& sWin, SyncedLifeEngine& eng, int& rAP, int mw, int mh, sf::RectangleShape& sb, sf::RectangleShape& chk, sf::RectangleShape& line, sf::CircleShape& sk, sf::Text& txt) {
  while (const std::optional<sf::Event> sev = sWin.pollEvent()) {
    if (sev->is<sf::Event::Closed>()) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.saveCurrentConfig();
      sWin.close();
      showSettings.store(false);
    }
  }
  bool pr = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
  if (!pr) eng.activeSliderId = SyncedLifeEngine::ID_NONE;
  static sf::Clock uiRC;
  if (uiRC.getElapsedTime().asMilliseconds() > 16) {
    uiRC.restart();
    sWin.clear({25, 25, 25});
    sf::Vector2i m = sf::Mouse::getPosition(sWin);
    bool canI = sWin.hasFocus();
    int oY = 10;
    Config cCopy;
    {
      std::lock_guard<std::mutex> lock(x11_mutex);
      cCopy = eng.getCfg();
    }
    bool localIsRussian = isRussian.load();
    float fR = static_cast<float>(cCopy.res), fG = static_cast<float>(cCopy.gap), fRn = static_cast<float>(cCopy.round), fS = static_cast<float>(500 - cCopy.speed),
          fA = static_cast<float>(cCopy.aggression), fMT = static_cast<float>(cCopy.max_teams), fFps = static_cast<float>(cCopy.fps),
          fC = static_cast<float>(cCopy.convex_power), fF = static_cast<float>(cCopy.fade_ms);
    auto drawH = [&](int y, const char* ru, const char* en) {
      txt.setCharacterSize(10);
      txt.setStyle(sf::Text::Bold);
      txt.setFillColor({0, 255, 170, 150});
      txt.setString(localIsRussian ? toUtf8(ru) : en);
      txt.setPosition({25.f, static_cast<float>(y + oY)});
      sWin.draw(txt);
      line.setSize({330.f, 1.f});
      line.setPosition({25.f, static_cast<float>(y + oY) + 18.f});
      line.setFillColor({0, 255, 170, 75});
      sWin.draw(line);
    };
    auto drawS = [&](int y, float v, float minV, float maxV, const char* ru, const char* en, const char* u, SyncedLifeEngine::SliderID slider_id, float st = 1.0f, bool is_d = false) -> std::pair<bool, float> {
      bool changed = false;
      float sW = 330.f, sX = 25.f, cY = static_cast<float>(y + oY);
      sf::FloatRect sA({sX - 10.f, cY + 10.f}, {sW + 20.f, 20.f});
      if (!is_d && canI && pr) {
        if (eng.activeSliderId == SyncedLifeEngine::ID_NONE && sA.contains(static_cast<sf::Vector2f>(m))) eng.activeSliderId = slider_id;
        if (eng.activeSliderId == slider_id) {
          float candidateV = std::clamp(std::round((minV + (std::clamp((static_cast<float>(m.x) - sX) / sW, 0.f, 1.f)) * (maxV - minV)) / st) * st, minV, maxV);
          if (candidateV != v) {
            v = candidateV;
            changed = true;
          }
        }
      }
      sb.setSize({sW, 1.f});
      sb.setPosition({sX, cY + 22.f});
      sb.setFillColor(is_d ? sf::Color(50, 50, 50) : sf::Color(80, 80, 80));
      sk.setPosition({sX + ((v - minV) / (maxV - minV)) * sW, cY + 22.f});
      sk.setFillColor(is_d ? sf::Color(65, 65, 65) : sf::Color(0, 120, 215));
      float disp = (slider_id == SyncedLifeEngine::ID_BEVEL) ? v * 100.f : v;
      txt.setCharacterSize(13);
      txt.setStyle(sf::Text::Regular);
      std::string labelStr = std::string(localIsRussian ? ru : en) + ": " + std::to_string(static_cast<int>(disp)) + " " + u;
      txt.setString(toUtf8(labelStr));
      txt.setPosition({sX, cY});
      txt.setFillColor(is_d ? sf::Color(100, 100, 100) : sf::Color(230, 230, 230));
      sWin.draw(txt);
      sWin.draw(sb);
      sWin.draw(sk);
      return {changed, v};
    };
    auto drawC = [&](float x, int y, bool v, const char* ru, const char* en, bool is_d = false) -> std::pair<bool, bool> {
      bool changed = false;
      float cY = static_cast<float>(y + oY);
      chk.setSize({18.f, 18.f});
      chk.setPosition({x, cY + 2.f});
      chk.setOutlineThickness(1);
      chk.setOutlineColor(is_d ? sf::Color(60, 60, 60) : sf::Color(100, 100, 100));
      chk.setFillColor(v ? (is_d ? sf::Color(40, 40, 40) : sf::Color(0, 120, 215)) : sf::Color::Transparent);
      sf::FloatRect boxHitbox({x, cY + 2.f}, {18.f, 18.f});
      if (!is_d && canI && pr && !wasP && boxHitbox.contains(sf::Vector2f{static_cast<float>(m.x), static_cast<float>(m.y)})) {
        v = !v;
        changed = true;
      }
      txt.setString(localIsRussian ? toUtf8(ru) : en);
      txt.setCharacterSize(12);
      txt.setPosition({x + 26.f, cY + 2.f});
      txt.setFillColor(is_d ? sf::Color(80, 80, 80) : sf::Color::White);
      sWin.draw(chk);
      sWin.draw(txt);
      return {changed, v};
    };
    static bool uiLoaded = false;
    static bool uiLastLang = localIsRussian;
    drawH(10, "ПАРАМЕТРЫ МИРА:", "WORLD SETTINGS:");
    if (auto [ch, val] = drawS(35, fS, 0, 500, "Скорость симуляции", "Simulation speed", "", SyncedLifeEngine::ID_SPD); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().speed = 500 - static_cast<int>(val);
    }
    if (auto [ch, val] = drawS(70, fA, 1, 8, "Агрессия команд", "Team aggression", "", SyncedLifeEngine::ID_AGGR); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().aggression = static_cast<int>(val);
    }
    if (auto [ch, val] = drawS(105, cCopy.mutation_chance, 0.f, 50.f, "Вероятность мутации", "Mutation chance", "%", SyncedLifeEngine::ID_MUT, 1.0f); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().mutation_chance = val;
    }
    if (auto [ch, val] = drawS(140, fMT, 0, 20, "Лимит команд", "Team limit", "", SyncedLifeEngine::ID_TEAMS); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().max_teams = static_cast<int>(val);
    }
    if (auto [ch, val] = drawS(175, fFps, 15, 144, "Максимальный FPS", "Max FPS", "", SyncedLifeEngine::ID_FPS); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().fps = static_cast<int>(val);
    }
    bool needLiveAtlasUpdate = false;
    drawH(220, "ВИЗУАЛИЗАЦИЯ КЛЕТОК:", "CELL VISUALS:");
    if (auto [ch, val] = drawS(245, fR, 4, 100, "Размер клетки", "Cell size", "px", SyncedLifeEngine::ID_RES); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().res = static_cast<int>(val);
      eng.glowWeightsNeedUpdate = true;
    }
    int currentResValid = std::max(4, cCopy.res);
    int maxGap = currentResValid - 2;
    if (auto [ch, val] = drawS(280, fG, -50.f, 50.f, "Зазор между клетками", "Cell gap", "px", SyncedLifeEngine::ID_GAP); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().gap = std::clamp(static_cast<int>(val), -50, maxGap);
    }
    if (auto [ch, val] = drawS(315, fRn, 0.f, 100.f, "Скругление клеток", "Cell rounding", "px", SyncedLifeEngine::ID_ROUND); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().round = static_cast<int>(val);
      eng.glowWeightsNeedUpdate = true;
      needLiveAtlasUpdate = true;
    }
    if (auto [ch, val] = drawS(350, cCopy.bevel_ratio, 0.f, 1.f, "Ширина фаски", "Bevel width", "%", SyncedLifeEngine::ID_BEVEL, 1.f / 330.f); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().bevel_ratio = std::clamp(val, 0.f, 1.f);
      eng.glowWeightsNeedUpdate = true;
      needLiveAtlasUpdate = true;
    }
    if (auto [ch, val] = drawS(385, fC, 0.f, 150.f, "Объем фаски", "Bevel extrusion", "", SyncedLifeEngine::ID_CONVEX, 1.f, (cCopy.bevel_ratio == 0.f)); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().convex_power = static_cast<int>(val);
      eng.glowWeightsNeedUpdate = true;
      needLiveAtlasUpdate = true;
    }
    if (needLiveAtlasUpdate) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.generateCellAtlas();
      eng.updateColorCache(eng.getCurrentDim());
      eng.worldChanged = true;
      eng.needsResort = true;
    }
    drawH(430, "ЭФФЕКТЫ:", "VISUAL EFFECTS:");
    if (auto [ch, val] = drawS(455, cCopy.glow_sigma, 0.f, 100.f, "Свечение клеток", "Cell glow", "", SyncedLifeEngine::ID_GLOW); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().glow_sigma = val;
      eng.glowWeightsNeedUpdate = true;
    }
    bool gA = (cCopy.glow_sigma > 0.1f);
    if (!gA && cCopy.only_glow) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().only_glow = false;
    }
    if (auto [ch, val] = drawC(25, 490, cCopy.only_glow, "Только свечение", "Only glow", !gA); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().only_glow = val;
    }
    if (auto [ch, val] = drawC(175, 490, cCopy.chromatic_aberration, "Хроматическая аберрация", "Glow aberration", !gA); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().chromatic_aberration = val;
      eng.glowWeightsNeedUpdate = true;
    }
    if (auto [ch, val] = drawS(525, fF, 0, 5000, "Плавное угасание клеток", "Cell fade out", "ms", SyncedLifeEngine::ID_FADE, 10.f); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().fade_ms = static_cast<int>(val);
    }
    if (auto [ch, val] = drawC(25, 560, cCopy.alpha_fade, "Прозрачность затухания", "Fade Transparency", (cCopy.fade_ms == 0)); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().alpha_fade = val;
    }
    if (auto [ch, val] = drawC(220, 560, cCopy.dust, "Частицы пыли", "Dust particles"); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().dust = val;
    }
    drawH(600, "ДИНАМИЧЕСКИЙ ФОН:", "DYNAMIC BACKGROUND:");
    if (auto [ch, val] = drawS(625, cCopy.bgValue, 0.f, 100.f, "Яркость фона", "Background brightness", "%", SyncedLifeEngine::ID_BGV); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().bgValue = val;
      eng.currentRuntimeBgColor = fromHSV(eng.currentBgHue, eng.getCfg().bgSaturation / 100.f, eng.getCfg().bgValue / 100.f);
    }
    if (auto [ch, val] = drawS(660, cCopy.bgSaturation, 0.f, 100.f, "Насыщенность фона", "Background saturation", "%", SyncedLifeEngine::ID_BGS, 1.f, (cCopy.bgValue == 0.f)); ch) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      eng.getCfg().bgSaturation = val;
      eng.currentRuntimeBgColor = fromHSV(eng.currentBgHue, eng.getCfg().bgSaturation / 100.f, eng.getCfg().bgValue / 100.f);
    }
    if (!pr && wasP) {
      bool needFullRestart = false;
      {
        std::lock_guard<std::mutex> lock(x11_mutex);
        eng.generateCellAtlas();
        eng.needsResort = true;
        if (eng.getCfg().res != rAP)
          needFullRestart = true;
        else
          eng.saveWorldState();
      }
      if (needFullRestart) {
        std::error_code ec;
        std::string p = get_cfg_path("world.dat");
        if (std::filesystem::exists(p, ec)) std::filesystem::remove(p, ec);
        keepColorsOnRestart.store(true);
        needRestart.store(true);
        rAP = cCopy.res;
      }
      eng.activeSliderId = SyncedLifeEngine::ID_NONE;
    }
    wasP = pr;
    sWin.display();
  }
}
void sync_menu_text(GtkWidget** wi) {
  auto sl = [&](int i, const char* ru, const char* en) {
    gtk_menu_item_set_label(GTK_MENU_ITEM(wi[i]), isRussian ? ru : en);
  };
  auto set_a = [](GtkWidget* w, bool a) {
    g_signal_handlers_block_matched(w, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, NULL);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w), a);
    g_signal_handlers_unblock_matched(w, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, NULL);
  };
  sl(0, "Параметры мира...", "Settings...");
  sl(1, "Перезапустить мир", "Restart world");
  sl(2, "Пауза", "Pause");
  sl(3, "Автопауза (игры / видео)", "Auto-pause (games / video)");
  sl(4, "Эффект параллакса", "Parallax effect");
  sl(5, "Взаимодействие мышкой", "Mouse interaction");
  sl(6, "Запускать при старте", "Run at startup");
  sl(7, "Запоминать состояние мира", "Persistent world");
  sl(8, "English Interface", "Русский интерфейс");
  sl(10, "Выход", "Exit");
  GtkWidget* label = gtk_bin_get_child(GTK_BIN(wi[0]));
  if (GTK_IS_LABEL(label)) {
    const char* t = gtk_menu_item_get_label(GTK_MENU_ITEM(wi[0]));
    char* m = g_strdup_printf("<b>%s</b>", t);
    gtk_label_set_markup(GTK_LABEL(label), m);
    g_free(m);
  }
  set_a(wi[2], isPaused.load());
  set_a(wi[6], IsAutoStartEnabled());
  if (globalEng) {
    Config& cfg = globalEng->getCfg();
    set_a(wi[3], cfg.auto_pause);
    set_a(wi[4], cfg.parallax_enabled);
    set_a(wi[5], cfg.mouse_enabled);
    set_a(wi[7], cfg.save_world);
  }
}
void run_gui() {
  while (!globalEng) std::this_thread::sleep_for(std::chrono::milliseconds(10));
  gtk_init_check(0, 0);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  GtkStatusIcon* icon = gtk_status_icon_new();
  if (!icon) {
    sf::err() << "[ERROR] [GUI] System tray is not available or not running. Status icon skipped." << std::endl;
  } else {
    std::string ic = get_cfg_path("icon.png");
    if (!fs::exists(ic)) {
      std::ofstream o(ic, std::ios::binary);
      o.write((char*)icon_png, icon_png_len);
    }
    gtk_status_icon_set_from_file(icon, ic.c_str());
    int max_attempts = 20;
    while (max_attempts > 0 && !gtk_status_icon_is_embedded(icon)) {
      while (g_main_context_iteration(NULL, FALSE));
      usleep(50000);
      max_attempts--;
    }
    if (!gtk_status_icon_is_embedded(icon)) {
      LOG_WARN("GUI", "Status icon created but failed to embed into the system tray. Activating settings window fallback.");
      showSettings = true;
    }
  }
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
  GtkWidget* menu = gtk_menu_new();
  static GtkWidget* wi[11];
  for (int i = 0; i < 11; i++) {
    wi[i] = (i >= 2 && i <= 7) ? gtk_check_menu_item_new() : gtk_menu_item_new();
  }
  sync_menu_text(wi);
  auto conn = [&](int i, auto f, gpointer d) { g_signal_connect(wi[i], "activate", G_CALLBACK(+f), d); };
  conn(0, [](GtkWidget*, gpointer) { showSettings = true; }, 0);
  conn(1, [](GtkWidget*, gpointer) { needRestart = true; }, 0);
  conn(2, [](GtkWidget* x, gpointer d) {
isPaused = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(x));
if (globalEng) {
std::lock_guard<std::mutex> lock(x11_mutex);
if (!isPaused && autoPaused) { autoPaused = false; }
if (!isPaused) {
globalEng->needsResort = true;
globalEng->glowWeightsNeedUpdate = true;
}
globalEng->saveCurrentConfig();
}
sync_menu_text((GtkWidget**)d); }, wi);
  conn(3, [](GtkWidget* x, gpointer d) {
if (globalEng) {
{
std::lock_guard<std::mutex> lock(x11_mutex);
globalEng->getCfg().auto_pause = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(x));
if (!globalEng->getCfg().auto_pause && autoPaused) {
isPaused = false; autoPaused = false;
globalEng->needsResort = true;
globalEng->glowWeightsNeedUpdate = true;
}
globalEng->saveCurrentConfig();
}
sync_menu_text((GtkWidget**)d);
} }, wi);
  conn(4, [](GtkWidget* x, gpointer d) {
if (globalEng) {
{
std::lock_guard<std::mutex> lock(x11_mutex);
globalEng->getCfg().parallax_enabled = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(x));
globalEng->saveCurrentConfig();
}
keepColorsOnRestart.store(true);
needRestart = true;
sync_menu_text((GtkWidget**)d);
} }, wi);
  conn(5, [](GtkWidget* x, gpointer d) {
if (globalEng) {
{
std::lock_guard<std::mutex> lock(x11_mutex);
globalEng->getCfg().mouse_enabled = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(x));
globalEng->saveCurrentConfig();
}
sync_menu_text((GtkWidget**)d);
} }, wi);
  conn(6, [](GtkWidget* x, gpointer d) {
SetAutoStart(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(x)));
sync_menu_text((GtkWidget**)d); }, wi);
  conn(7, [](GtkWidget* x, gpointer d) {
if (globalEng) {
{
std::lock_guard<std::mutex> lock(x11_mutex);
globalEng->getCfg().save_world = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(x));
globalEng->saveCurrentConfig();
}
sync_menu_text((GtkWidget**)d);
} }, wi);
  g_signal_connect(wi[8], "activate", G_CALLBACK(+[](GtkWidget* w, gpointer d) {
                     isRussian = !isRussian;
                     GtkWidget** items = (GtkWidget**)d;
                     sync_menu_text(items);
                     if (globalEng) {
                       std::lock_guard<std::mutex> lock(x11_mutex);
                       globalEng->saveCurrentConfig();
                     }
                   }),
                   wi);
  conn(10, [](GtkWidget*, gpointer) {
running = false;
if (globalEng) {
std::lock_guard<std::mutex> lock(x11_mutex);
globalEng->saveWorldState();
globalEng->saveCurrentConfig();
}
gtk_main_quit(); }, 0);
  int order[] = {0, -1, 1, 2, 3, -1, 4, 5, -1, 6, 7, -1, 8, -1, 10};
  static GtkWidget* separators[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
  int sepIdx = 0;
  for (int i : order) {
    if (i < 0) {
      if (sepIdx < 5 && !separators[sepIdx]) {
        separators[sepIdx] = gtk_separator_menu_item_new();
      }
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), separators[sepIdx++]);
    } else {
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), wi[i]);
    }
  }
  gtk_widget_show_all(menu);
  bool is_embedded = false;
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  if (icon && gtk_status_icon_is_embedded(icon)) {
    is_embedded = true;
  }
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
  if (is_embedded) {
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    g_signal_connect(icon, "popup-menu", G_CALLBACK(+[](GtkStatusIcon*, guint button, guint activate_time, gpointer menu_ptr) {
                       gtk_menu_popup_at_pointer(GTK_MENU(menu_ptr), NULL);
                     }),
                     menu);
    g_signal_connect(icon, "activate", G_CALLBACK(+[](GtkStatusIcon*) {
                       showSettings = true;
                     }),
                     0);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
  } else {
    showSettings = true;
  }
  gtk_main();
}
void mouse_polling_loop(Display* global_dpy, Window root, Window xid) {
  Display* dpy = XOpenDisplay(NULL);
  if (!dpy) return;
  Atom local_wm_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
  Atom local_wm_desktop = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
  while (running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    if (!globalEng) {
      lastMouseState = false;
      continue;
    }
    Config& c = globalEng->getCfg();
    if (!c.mouse_enabled || isPaused || autoPaused) {
      lastMouseState = false;
      continue;
    }
    Window rR, cR;
    int rx, ry, wx, wy;
    unsigned int mask;
    if (XQueryPointer(dpy, root, &rR, &cR, &rx, &ry, &wx, &wy, &mask)) {
      bool click = (mask & Button1Mask);
      if (click) {
        if (!lastMouseState) {
          bool overGUI = (showSettings);
          bool ok = (cR == 0 || cR == root || cR == xid);
          if (!ok && !overGUI) {
            Atom a_t;
            int a_f;
            unsigned long n = 0, b = 0;
            unsigned char* p = nullptr;
            if (XGetWindowProperty(dpy, cR, local_wm_type, 0, 1, False, XA_ATOM, &a_t, &a_f, &n, &b, &p) == 0 && p && n > 0) {
              if (*(Atom*)p == local_wm_desktop) ok = true;
              XFree(p);
            }
          }
          if (ok && !overGUI) {
            std::lock_guard<std::mutex> lock(x11_mutex);
            int res = std::max(1, c.res);
            sf::Vector2f camOff = globalEng->getCameraOffset();
            int tx = static_cast<int>(std::floor((static_cast<float>(rx) + camOff.x) / static_cast<float>(res))) + static_cast<int>(globalEng->cachedPadX);
            int ty = static_cast<int>(std::floor((static_cast<float>(ry) + camOff.y) / static_cast<float>(res))) + static_cast<int>(globalEng->cachedPadY);
            int cols = globalEng->getCols(), rows = globalEng->getRows();
            if (tx >= 0 && tx < cols && ty >= 0 && ty < rows) {
              int tId = 0;
              auto& aMap = globalEng->getAliveMap();
              auto& tMap = globalEng->getTeamMap();
              size_t mapBounds = aMap.size();
              for (int i = 0; i < 25; i++) {
                int nx = (tx + (i % 5) - 2 + cols) % cols;
                int ny = (ty + (i / 5) - 2 + rows) % rows;
                size_t ix = (size_t)ny * cols + nx;
                if (ix < mapBounds && aMap[ix]) {
                  tId = tMap[ix];
                  break;
                }
              }
              if (tId > 0) {
                globalEng->spawnCluster(tx, ty, tId);
                globalEng->worldChanged = true;
                globalEng->needsResort = true;
                if (isPaused) globalEng->glowWeightsNeedUpdate = true;
              }
            }
          }
        }
      }
      lastMouseState = click;
    } else {
      lastMouseState = false;
    }
  }
  XCloseDisplay(dpy);
}
bool isFullscreenWindowActive(Display* dpy, Window root) {
  if (ATOM_ACTIVE_WINDOW == 0 || ATOM_WM_STATE == 0 || ATOM_WM_FULLSCREEN == 0) return false;
  Window activeWindow = 0;
  Atom actualType;
  int actualFormat;
  unsigned long nItems = 0, bytesAfter = 0;
  unsigned char* prop = nullptr;
  if (XGetWindowProperty(dpy, root, ATOM_ACTIVE_WINDOW, 0, 1, 0, XA_WINDOW,
                         &actualType, &actualFormat, &nItems, &bytesAfter, &prop) == 0 &&
      prop) {
    if (nItems > 0) {
      activeWindow = *(Window*)prop;
    }
    XFree(prop);
    if (activeWindow != 0) {
      unsigned char* propState = nullptr;
      if (XGetWindowProperty(dpy, activeWindow, ATOM_WM_STATE, 0, 64, False, XA_ATOM,
                             &actualType, &actualFormat, &nItems, &bytesAfter, &propState) == 0 &&
          propState) {
        bool found = false;
        if (nItems > 0) {
          Atom* atoms = (Atom*)propState;
          for (unsigned long i = 0; i < nItems; i++) {
            if (atoms[i] == ATOM_WM_FULLSCREEN) {
              found = true;
              break;
            }
          }
        }
        XFree(propState);
        return found;
      }
    }
  }
  return false;
}
void signalHandler(int signum) {
  running = false;
}
int main(int argc, char** argv) {
  std::string lockPath = get_cfg_path("syntropy.lock");
  int lockFd = open(lockPath.c_str(), O_CREAT | O_RDWR, 0666);
  if (lockFd == -1 || flock(lockFd, LOCK_EX | LOCK_NB) == -1) return 0;
  std::string configPath = get_cfg_path("settings.cfg");
  bool isFirstRun = !fs::exists(configPath);
  std::signal(SIGSEGV, LinuxCrashHandler);
  std::signal(SIGFPE, LinuxCrashHandler);
  std::signal(SIGILL, LinuxCrashHandler);
  std::signal(SIGABRT, LinuxCrashHandler);
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGHUP, signalHandler);
  std::ofstream logFile(get_cfg_path("log.txt"));
  std::streambuf* previous = sf::err().rdbuf(logFile.rdbuf());
  if (isFirstRun) {
    std::string iconPath = get_cfg_path("icon.png");
    if (!fs::exists(iconPath)) {
      std::ofstream o(iconPath, std::ios::binary);
      o.write(reinterpret_cast<const char*>(icon_png), icon_png_len);
    }
    if (has("notify-send")) {
      std::string notifyCmd = "notify-send -u critical -i '" + iconPath + "' 'Syntropy' 'RU: Все настройки доступны через иконку в системном трее.\nEN: All settings are available via the system tray icon.' &";
      try {
        int res = std::system(notifyCmd.c_str());
        (void)res;
      } catch (...) {
        sf::err() << "[WARN] [System] Unknown exception during notification." << std::endl;
      }
    } else {
      sf::err() << "[INFO] [System] notify-send is missing, skipping desktop notification." << std::endl;
    }
  }
  if (XInitThreads() == 0) {
    LOG_ERROR("System", "Critical: XInitThreads failed! Multithreading in X11 is unsupported.");
    if (lockFd != -1) close(lockFd);
    return 1;
  }
  Display* dpy = XOpenDisplay(NULL);
  if (!dpy) {
    LOG_ERROR("System", "Critical: Cannot open X11 Display connection connection handle.");
    if (lockFd != -1) close(lockFd);
    return 1;
  }
  ATOM_ACTIVE_WINDOW = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
  ATOM_WM_STATE = XInternAtom(dpy, "_NET_WM_STATE", False);
  ATOM_WM_FULLSCREEN = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
  ATOM_WM_TYPE = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
  ATOM_WM_DESKTOP = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
  XSynchronize(dpy, False);
  Window root = DefaultRootWindow(dpy);
  if (!globalFont.openFromMemory(segoeui_ttf, segoeui_ttf_len)) {
    if (lockFd != -1) close(lockFd);
    return 1;
  }
  sf::RenderWindow win(sf::VideoMode({(unsigned int)mw, (unsigned int)mh}), "Syntropy", sf::Style::Close);
  setIcon(win);
  SyncedLifeEngine eng(mw, mh);
  globalEng = &eng;
  static int rAP = eng.getCfg().res;
  win.setFramerateLimit((unsigned int)eng.getCfg().fps);
  Window xid = win.getNativeHandle();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  sf::RenderTexture wallpaperBuffer;
  if (!wallpaperBuffer.resize({(unsigned int)mw, (unsigned int)mh})) return -1;
  sf::Sprite wallpaperBufferSpr(wallpaperBuffer.getTexture());
  std::thread gui_thread(run_gui);
  gui_thread.detach();
  std::atomic<bool> logic_running{true};
  std::thread simulation_thread([&]() {
    while (running && logic_running) {
      if (isPaused || autoPaused || needRestart.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        continue;
      }
      int speed = 0;
      {
        std::lock_guard<std::mutex> lock(x11_mutex);
        speed = eng.getCfg().speed;
        eng.update();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(speed > 0 ? speed : 1));
    }
  });
  simulation_thread.detach();
  sf::RenderWindow sWin;
  sf::RectangleShape sb({360.f, 2.f}), chk({16.f, 16.f}), line({330.f, 1.f});
  sf::CircleShape sk(7.f);
  sk.setFillColor({0, 120, 215});
  sk.setOrigin({7.f, 7.f});
  sf::Text txt(globalFont, "");
  sf::Clock xUC, pCC, startupProtectionClock;
  bool globalJustPaused = false;
  Display* pause_dpy = XOpenDisplay(NULL);
  while (win.isOpen() && running) {
    while (const std::optional<sf::Event> ev = win.pollEvent()) {
      if (ev->is<sf::Event::Closed>()) win.close();
    }
    if (showSettings) {
      if (!sWin.isOpen()) {
        unsigned int h = 730;
        sWin.create(sf::VideoMode({380, h}), toUtf8(isRussian ? L"Настройки" : L"Settings"), sf::Style::Close);
        setIcon(sWin);
        sWin.setFramerateLimit(60);
        rAP = eng.getCfg().res;
        sf::Vector2i winPos = win.getPosition();
        int tx = winPos.x + mw - 420;
        int ty = winPos.y + mh - (int)h - 80;
        sWin.setPosition({tx, ty});
      }
      renderSettingsWindow(sWin, eng, rAP, mw, mh, sb, chk, line, sk, txt);
    }
    if (needRestart) {
      std::lock_guard<std::mutex> lock(x11_mutex);
      std::error_code ec;
      std::string p = get_cfg_path("world.dat");
      if (fs::exists(p, ec)) fs::remove(p, ec);
      eng.restart(keepColorsOnRestart.load(), true);
      rAP = eng.getCfg().res;
      needRestart = false;
      keepColorsOnRestart.store(false);
      eng.worldChanged = true;
      eng.glowWeightsNeedUpdate = true;
    }
    bool autoPauseEnabled = false;
    {
      std::lock_guard<std::mutex> lock(x11_mutex);
      autoPauseEnabled = eng.getCfg().auto_pause;
    }
    if (autoPauseEnabled && pCC.getElapsedTime().asSeconds() > 1.0f) {
      bool wasAutoPaused = autoPaused;
      {
        std::lock_guard<std::mutex> lock(x11_mutex);
        autoPaused = isFullscreenWindowActive(pause_dpy, root);
        if (wasAutoPaused && !autoPaused) {
          eng.needsResort = true;
          eng.worldChanged = true;
          eng.glowWeightsNeedUpdate = true;
        }
      }
      pCC.restart();
    } else if (!autoPauseEnabled) {
      autoPaused = false;
    }
    if (win.hasFocus() && !isPaused && !autoPaused) {
      Config& c = eng.getCfg();
      if (c.mouse_enabled && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        sf::Vector2i localMouse = sf::Mouse::getPosition(win);
        if (localMouse.x >= 0 && localMouse.x < mw && localMouse.y >= 0 && localMouse.y < mh) {
          std::lock_guard<std::mutex> lock(x11_mutex);
          int res = std::max(1, c.res);
          sf::Vector2f camOff = eng.getCameraOffset();
          int tx = static_cast<int>(std::floor((static_cast<float>(localMouse.x) + camOff.x) / static_cast<float>(res))) + static_cast<int>(eng.cachedPadX);
          int ty = static_cast<int>(std::floor((static_cast<float>(localMouse.y) + camOff.y) / static_cast<float>(res))) + static_cast<int>(eng.cachedPadY);
          int cols = eng.getCols(), rows = eng.getRows();
          if (tx >= 0 && tx < cols && ty >= 0 && ty < rows) {
            int tId = 0;
            auto& aMap = eng.getAliveMap();
            auto& tMap = eng.getTeamMap();
            size_t mapBounds = aMap.size();
            for (int i = 0; i < 25; i++) {
              int nx = (tx + (i % 5) - 2 + cols) % cols;
              int ny = (ty + (i / 5) - 2 + rows) % rows;
              size_t ix = (size_t)ny * cols + nx;
              if (ix < mapBounds && aMap[ix]) {
                tId = tMap[ix];
                break;
              }
            }
            if (tId > 0) {
              eng.spawnCluster(tx, ty, tId);
              eng.worldChanged = true;
              eng.needsResort = true;
            }
          }
        }
      }
    }
    unsigned int currentConfigFps = 60;
    {
      std::lock_guard<std::mutex> lock(x11_mutex);
      currentConfigFps = std::max(15u, (unsigned int)eng.getCfg().fps);
    }
    static unsigned int lastAppliedFps = 0;
    if (currentConfigFps != lastAppliedFps) {
      win.setFramerateLimit(currentConfigFps);
      lastAppliedFps = currentConfigFps;
    }
    bool paused = (isPaused || autoPaused);
    static sf::Clock wallpaper_limit_clk;
    float target_fps = std::max(1.f, (float)currentConfigFps);
    bool time_for_wallpaper_frame = (wallpaper_limit_clk.getElapsedTime().asSeconds() >= (1.0f / target_fps));
    bool need_window_refresh_local = false;
    if (!paused && time_for_wallpaper_frame) {
      wallpaper_limit_clk.restart();
      std::lock_guard<std::mutex> lock(x11_mutex);
      wallpaperBuffer.clear(eng.currentRuntimeBgColor);
      eng.render(wallpaperBuffer, win);
      wallpaperBuffer.display();
      need_window_refresh_local = true;
      globalJustPaused = false;
    } else if (paused) {
      bool settingsChanged = false;
      {
        std::lock_guard<std::mutex> lock(x11_mutex);
        settingsChanged = (eng.worldChanged || eng.glowWeightsNeedUpdate || needAtlasUpdate);
      }
      if (!globalJustPaused || settingsChanged) {
        std::lock_guard<std::mutex> lock(x11_mutex);
        wallpaperBuffer.clear(eng.currentRuntimeBgColor);
        eng.render(wallpaperBuffer, win);
        eng.worldChanged = false;
        eng.glowWeightsNeedUpdate = false;
        wallpaperBuffer.display();
        globalJustPaused = true;
        need_window_refresh_local = true;
      }
    }
    if (need_window_refresh_local) {
      win.clear(eng.currentRuntimeBgColor);
      win.draw(wallpaperBufferSpr);
      win.display();
    } else {
      unsigned int sleep_ms = 1000 / currentConfigFps;
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
  }
  logic_running = false;
  if (globalEng) {
    std::lock_guard<std::mutex> lock(x11_mutex);
    globalEng->saveWorldState();
    globalEng->saveCurrentConfig();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  if (lockFd != -1) {
    flock(lockFd, LOCK_UN);
    close(lockFd);
    std::error_code ec;
    fs::remove(lockPath, ec);
  }
  if (dpy) {
    std::lock_guard<std::mutex> lock(x11_mutex);
    XCloseDisplay(dpy);
  }
  if (pause_dpy) {
    XCloseDisplay(pause_dpy);
  }
  return 0;
}
