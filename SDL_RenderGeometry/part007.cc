// part 007: Box swarm, and printing metadata text
//
// Visual is a swarm of boxes that can be jerked around with WASD keys.
// The 'P' key can be used to toggle perspective.
//
// Also, this is the first file to use the assets/RobotoMono-24.bmp file
// to render some metadata text on the screen.
//

#include <SDL2/SDL.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

[[noreturn]] inline void panic(const std::string_view &message) {
  std::cerr << "PANIC: " << message << std::endl;
  std::exit(1);
}

[[noreturn]] inline void sdlError(const std::string_view &tag) {
  std::cerr << "ERROR " << tag << ": " << SDL_GetError() << std::endl;
  std::exit(1);
}

inline void assertThat(bool condition) {
  if (!condition) {
    panic("ASSERTION FAILED");
  }
}

constexpr const bool DEBUG = true;
constexpr const int WIDTH = 800;
constexpr const int HEIGHT = 600;
constexpr const float FPS = 60.0;
constexpr const float SEC_PER_FRAME = 1 / FPS;
constexpr const Uint64 MS_PER_FRAME = Uint64(1000 * SEC_PER_FRAME + 1);
constexpr const char FONT_SHEET_PATH[] = "assets/RobotoMono-24.bmp";

constexpr const float PI = M_PI;
constexpr const float TAU = 2 * PI;

// PICO-8 Colors
using Color = SDL_Color;
constexpr const Color BLACK = Color{0, 0, 0, 255};               // 0
constexpr const Color DARK_BLUE = Color{29, 43, 83, 255};        // 1
constexpr const Color DARK_PURPLE = Color{126, 37, 83, 255};     // 2
constexpr const Color DARK_GREEN = Color{0, 135, 81, 255};       // 3
constexpr const Color BROWN = Color{171, 82, 54, 255};           // 4
constexpr const Color DARK_GREY = Color{95, 87, 79, 255};        // 5
constexpr const Color LIGHT_GREY = Color{194, 195, 199, 255};    // 6
constexpr const Color WHITE = Color{255, 241, 232, 255};         // 7
constexpr const Color RED = Color{255, 0, 77, 255};              // 8
constexpr const Color ORANGE = Color{255, 163, 0, 255};          // 9
constexpr const Color YELLOW = Color{255, 236, 39, 255};         // 10
constexpr const Color GREEN = Color{0, 228, 54, 255};            // 11
constexpr const Color BLUE = Color{41, 173, 255, 255};           // 12
constexpr const Color LAVENDER = Color{131, 118, 156, 255};      // 13
constexpr const Color PINK = Color{255, 119, 168, 255};          // 14
constexpr const Color LIGHT_PEACH = Color{255, 204, 170, 255};   // 15
constexpr const Color BROWNISH_BLACK = Color{41, 24, 20, 255};   // 128 / -16
constexpr const Color DARKER_BLUE = Color{17, 29, 53, 255};      // 129 / -15
constexpr const Color DARKER_PURPLE = Color{66, 33, 54, 255};    // 130 / -14
constexpr const Color BLUE_GREEN = Color{18, 83, 89, 255};       // 131 / -13
constexpr const Color DARK_BROWN = Color{116, 47, 41, 255};      // 132 / -12
constexpr const Color DARKER_GREY = Color{73, 51, 59, 255};      // 133 / -11
constexpr const Color MEDIUM_GREY = Color{162, 136, 121, 255};   // 134 / -10
constexpr const Color LIGHT_YELLOW = Color{243, 239, 125, 255};  // 135 / -9
constexpr const Color DARK_RED = Color{190, 18, 80, 255};        // 136 / -8
constexpr const Color DARK_ORANGE = Color{255, 108, 36, 255};    // 137 / -7
constexpr const Color LIME_GREEN = Color{168, 231, 46, 255};     // 138 / -6
constexpr const Color MEDIUM_GREEN = Color{0, 181, 67, 255};     // 139 / -5
constexpr const Color TRUE_BLUE = Color{6, 90, 181, 255};        // 140 / -4
constexpr const Color MAUVE = Color{117, 70, 101, 255};          // 141 / -3
constexpr const Color DARK_PEACH = Color{255, 110, 89, 255};     // 142 / -2
constexpr const Color PEACH = Color{255, 157, 129, 255};         // 143 / -1
constexpr const SDL_Color COLORS[] = {
    BLACK,       DARK_BLUE,   DARK_PURPLE,    DARK_GREEN,  BROWN,         DARK_GREY,  LIGHT_GREY,
    WHITE,       RED,         ORANGE,         YELLOW,      GREEN,         BLUE,       LAVENDER,
    PINK,        LIGHT_PEACH, BROWNISH_BLACK, DARKER_BLUE, DARKER_PURPLE, BLUE_GREEN, DARK_BROWN,
    DARKER_GREY, MEDIUM_GREY, LIGHT_YELLOW,   DARK_RED,    DARK_ORANGE,   LIME_GREEN, MEDIUM_GREEN,
    TRUE_BLUE,   MAUVE,       DARK_PEACH,     PEACH,
};

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *boxTexture;
static SDL_Texture *fontSheet;
static int charHeight, charWidth, charX, charY;

inline void init(int w, int h) {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    sdlError("SDL_Init");
  }
  window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, 0);
  if (!window) {
    sdlError("SDL_CreateWindow");
  }
  renderer = SDL_CreateRenderer(
      window, -1,
      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
  if (!renderer) {
    sdlError("SDL_CreateRenderer");
  }
  {
    auto surface = SDL_CreateRGBSurfaceWithFormat(0, 128, 128, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) {
      sdlError("SDL_CreateRGBSurfaceWithFormat");
    }
    auto pixels = static_cast<SDL_Color *>(surface->pixels);
    for (int i = 0; i < 128; i++) {
      for (int j = 0; j < 128; j++) {
        auto &pixel = pixels[128 * i + j];
        pixel.a = 255;
        if (i == 0 || i == 127 || j == 0 || j == 127) {
          pixel.r = pixel.g = pixel.b = 0;
        } else {
          pixel.r = pixel.g = pixel.b = 255;
        }
      }
    }
    boxTexture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!boxTexture) {
      sdlError("SDL_CreateTextureFromSurface");
    }
    SDL_FreeSurface(surface);
  }
  {
    auto surface = SDL_LoadBMP(FONT_SHEET_PATH);
    if (!surface) {
      sdlError("SDL_LoadBMP");
    }
    charHeight = surface->h;
    charWidth = surface->w / 128;
    charX = charWidth;
    charY = charHeight;
    if (surface->w % 128 != 0) {
      std::cerr << "surface->w = " << surface->w << std::endl;
      panic("Unexpected font sheet width");
    }
    fontSheet = SDL_CreateTextureFromSurface(renderer, surface);
    if (!fontSheet) {
      sdlError("SDL_CreateTextureFromSurface");
    }
    SDL_FreeSurface(surface);
  }
}

inline void clear(Color color) {
  if (SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a) != 0) {
    sdlError("SDL_SetRenderDrawColor");
  }
  if (SDL_RenderClear(renderer) != 0) {
    sdlError("SDL_RenderClear");
  }
}

inline void setFontColor(Uint8 r, Uint8 g, Uint8 b) {
  if (SDL_SetTextureColorMod(fontSheet, r, g, b) != 0) {
    sdlError("SDL_SetTextureColorMod");
  }
}

inline void setFontColor(Color color) { setFontColor(color.r, color.g, color.b); }

inline void printChar(int x, int y, char ch) {
  auto src = SDL_Rect{charWidth * (int)((Uint8)ch >= 128 ? ' ' : ch), 0, charWidth, charHeight};
  auto dst = SDL_Rect{x, y, charWidth, charHeight};
  if (SDL_RenderCopy(renderer, fontSheet, &src, &dst) != 0) {
    sdlError("SDL_RenderCopy (printChar)");
  }
}

inline void printStringView(const std::string_view &view) {
  for (auto ch : view) {
    printChar(charX, charY, ch);
    if (ch == '\n') {
      charX = charWidth;
      charY += charHeight;
    } else {
      charX += charWidth;
    }
  }
}

template <class T>
inline void printConvertible(const T &t) {
  if constexpr (std::is_convertible_v<const T &, std::string_view>) {
    printStringView(std::string_view(t));
  } else {
    std::stringstream ss;
    ss << t;
    printStringView(ss.str());
  }
}

template <class... Args>
inline void print(const Args &...args) {
  (printConvertible(args), ...);
}

struct Vector final {
  float x, y, z, w;
};

inline Vector operator+(const Vector &v) { return v; }
inline Vector operator-(const Vector &v) { return Vector{-v.x, -v.y, -v.z, -v.w}; }

inline Vector &operator*=(Vector &lhs, const float &rhs) {
  lhs.x *= rhs;
  lhs.y *= rhs;
  lhs.z *= rhs;
  lhs.w *= rhs;
  return lhs;
}

inline Vector operator*(const Vector &lhs, const float &rhs) {
  return Vector{lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs};
}

inline Vector &operator/=(Vector &lhs, const float &rhs) {
  lhs.x /= rhs;
  lhs.y /= rhs;
  lhs.z /= rhs;
  lhs.w /= rhs;
  return lhs;
}

inline Vector operator/(const Vector &lhs, const float &rhs) {
  return Vector{lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs};
}

inline Vector &operator+=(Vector &lhs, const Vector &rhs) {
  lhs.x += rhs.x;
  lhs.y += rhs.y;
  lhs.z += rhs.z;
  lhs.w += rhs.w;
  return lhs;
}

inline Vector operator+(const Vector &lhs, const Vector &rhs) {
  return Vector{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

inline Vector &operator-=(Vector &lhs, const Vector &rhs) {
  lhs.x -= rhs.x;
  lhs.y -= rhs.y;
  lhs.z -= rhs.z;
  lhs.w -= rhs.w;
  return lhs;
}

inline Vector operator-(const Vector &lhs, const Vector &rhs) {
  return Vector{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

inline float dot(const Vector &lhs, const Vector &rhs) {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

inline float lengthOf(const Vector &v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

inline Vector normalize(const Vector &v, float newLength = 1.0f) {
  return v * (newLength / lengthOf(v));
}

inline std::ostream &operator<<(std::ostream &out, const Vector &v) {
  return out << "{" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "}";
}

struct Matrix final {
  // x = row1, y = row2, z = row3, w = row4
  // Kind of a hack, but using the same names as Vector allows me to
  // just copy and paste the implementations for various operators from Vector
  // and just reuse them for Matrix by replacing "Vector" with "Matrix".
  Vector x, y, z, w;
};

inline Vector xColumn(const Matrix &m) { return Vector{m.x.x, m.y.x, m.z.x, m.w.x}; }
inline Vector yColumn(const Matrix &m) { return Vector{m.x.y, m.y.y, m.z.y, m.w.y}; }
inline Vector zColumn(const Matrix &m) { return Vector{m.x.z, m.y.z, m.z.z, m.w.z}; }
inline Vector wColumn(const Matrix &m) { return Vector{m.x.w, m.y.w, m.z.w, m.w.w}; }

inline Matrix operator+(const Matrix &m) { return m; }
inline Matrix operator-(const Matrix &m) { return Matrix{-m.x, -m.y, -m.z, -m.w}; }

inline Matrix &operator*=(Matrix &lhs, const float &rhs) {
  lhs.x *= rhs;
  lhs.y *= rhs;
  lhs.z *= rhs;
  lhs.w *= rhs;
  return lhs;
}

inline Matrix operator*(const Matrix &lhs, const float &rhs) {
  return Matrix{lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs};
}

inline Matrix &operator/=(Matrix &lhs, const float &rhs) {
  lhs.x /= rhs;
  lhs.y /= rhs;
  lhs.z /= rhs;
  lhs.w /= rhs;
  return lhs;
}

inline Matrix operator/(const Matrix &lhs, const float &rhs) {
  return Matrix{lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs};
}

inline Matrix &operator+=(Matrix &lhs, const Matrix &rhs) {
  lhs.x += rhs.x;
  lhs.y += rhs.y;
  lhs.z += rhs.z;
  lhs.w += rhs.w;
  return lhs;
}

inline Matrix operator+(const Matrix &lhs, const Matrix &rhs) {
  return Matrix{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

inline Matrix &operator-=(Matrix &lhs, const Matrix &rhs) {
  lhs.x -= rhs.x;
  lhs.y -= rhs.y;
  lhs.z -= rhs.z;
  lhs.w -= rhs.w;
  return lhs;
}

inline Matrix operator-(const Matrix &lhs, const Matrix &rhs) {
  return Matrix{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

inline Vector operator*(const Matrix &lhs, const Vector &rhs) {
  return Vector{dot(lhs.x, rhs), dot(lhs.y, rhs), dot(lhs.z, rhs), dot(lhs.w, rhs)};
}

inline Matrix operator*(const Matrix &lhs, const Matrix &rhs) {
  constexpr const auto &x = xColumn;
  constexpr const auto &y = yColumn;
  constexpr const auto &z = zColumn;
  constexpr const auto &w = wColumn;
  return Matrix{
      {dot(lhs.x, x(rhs)), dot(lhs.x, y(rhs)), dot(lhs.x, z(rhs)), dot(lhs.x, w(rhs))},  //
      {dot(lhs.y, x(rhs)), dot(lhs.y, y(rhs)), dot(lhs.y, z(rhs)), dot(lhs.y, w(rhs))},  //
      {dot(lhs.z, x(rhs)), dot(lhs.z, y(rhs)), dot(lhs.z, z(rhs)), dot(lhs.z, w(rhs))},  //
      {dot(lhs.w, x(rhs)), dot(lhs.w, y(rhs)), dot(lhs.w, z(rhs)), dot(lhs.w, w(rhs))},  //
  };
}

inline Matrix &operator*=(Matrix &lhs, const Matrix &rhs) {
  lhs = lhs * rhs;
  return lhs;
}

inline constexpr const Matrix IDENTITY = Matrix{
    {1.0, 0.0, 0.0, 0.0},
    {0.0, 1.0, 0.0, 0.0},
    {0.0, 0.0, 1.0, 0.0},
    {0.0, 0.0, 0.0, 1.0},
};

inline constexpr Matrix translation(float x, float y, float z) {
  return Matrix{
      {1, 0, 0, x},
      {0, 1, 0, y},
      {0, 0, 1, z},
      {0, 0, 0, 1},
  };
}

inline constexpr Matrix translation(const Vector &v) { return translation(v.x, v.y, v.z); }

inline constexpr Matrix scaling(float x, float y, float z) {
  return Matrix{
      {x, 0, 0, 0},
      {0, y, 0, 0},
      {0, 0, z, 0},
      {0, 0, 0, 1},
  };
}
inline constexpr Matrix scaling(const Vector &v) { return scaling(v.x, v.y, v.z); }

inline Matrix xRotation(float radians) {
  auto c = std::cos(radians);
  auto s = std::sin(radians);
  return Matrix{
      {1, 0, 0, 0},
      {0, c, -s, 0},
      {0, s, c, 0},
      {0, 0, 0, 1},
  };
}

inline Matrix yRotation(float radians) {
  auto c = std::cos(radians);
  auto s = std::sin(radians);
  return Matrix{
      {c, 0, s, 0},
      {0, 1, 0, 0},
      {-s, 0, c, 0},
      {0, 0, 0, 1},
  };
}

inline Matrix zRotation(float radians) {
  auto c = std::cos(radians);
  auto s = std::sin(radians);
  return Matrix{
      {c, -s, 0, 0},
      {s, c, 0, 0},
      {0, 0, 1, 0},
      {0, 0, 0, 1},
  };
}

inline Matrix rotation(float x, float y, float z) {
  return xRotation(x) * yRotation(y) * zRotation(z);
}

inline Matrix rotation(Vector radians) { return rotation(radians.x, radians.y, radians.z); }

inline constexpr Matrix viewport(float width = WIDTH, float height = HEIGHT) {
  return Matrix{
      {width / 2.0f, 0, 0, width / 2.0f},
      {0, -height / 2.0f, 0, height / 2.0f},
      {0, 0, -1, 0},
      {0, 0, 0, 1},
  };
}

inline constexpr Matrix perspective(float fov, float aspect, float near, float far) {
  auto f = 1 / std::tan(fov / 2);
  auto nf = 1 / (near - far);
  return Matrix{
      {f / aspect, 0, 0, 0},
      {0, f, 0, 0},
      {0, 0, (far + near) * nf, 2 * far * near * nf},
      {0, 0, -1, 0},
  };
}

struct RenderState final {
  Matrix transform;
  Color fillColor;
};

static RenderState state = {.transform = IDENTITY, .fillColor = WHITE};
static std::vector<RenderState> stack;

inline void push() { stack.push_back(state); }
inline void pop() {
  assertThat(stack.size());
  state = stack.back();
  stack.pop_back();
}

inline void apply(const Matrix &matrix) { state.transform *= matrix; }

struct Vertex final {
  float x, y, z;  // screen coordinates
  Color color;    // modulating color
  float u, v;     // normalized texture coordinates
};

struct Triangle final {
  Vertex vertices[3];
};

static_assert(sizeof(Vertex) == 6 * sizeof(float));
static_assert(offsetof(Vertex, x) == 0 * sizeof(float));
static_assert(offsetof(Vertex, y) == 1 * sizeof(float));
static_assert(offsetof(Vertex, z) == 2 * sizeof(float));
static_assert(offsetof(Vertex, color) == 3 * sizeof(float));
static_assert(offsetof(Vertex, u) == 4 * sizeof(float));
static_assert(offsetof(Vertex, v) == 5 * sizeof(float));
static_assert(sizeof(Triangle) == 3 * sizeof(Vertex));

static std::vector<Triangle> triangles;

inline void perspectiveDivide(Vector &v) {
  v.x /= v.w;
  v.y /= v.w;
  v.z /= v.w;
  v.w = 1.0;
}

inline void addTriangle(const Vector &ia, const Vector &ib, const Vector &ic, float u1, float v1,
                        float u2, float v2, float u3, float v3) {
  auto a = state.transform * ia;
  auto b = state.transform * ib;
  auto c = state.transform * ic;
  if (a.w <= 0 || b.w <= 0 || c.w <= 0) {
    return;  // If any vertex of the triangle is behind the camera, skip it.
  }
  perspectiveDivide(a);
  perspectiveDivide(b);
  perspectiveDivide(c);
  if (a.x < 0 && b.x < 0 && c.x < 0) {
    return;  // entirely on other side of X = 0 plane
  }
  if (a.x > WIDTH && b.x > WIDTH && c.x > WIDTH) {
    return;  // entirely on other side of X = WIDTH plane
  }
  if (a.y < 0 && b.y < 0 && c.y < 0) {
    return;  // entirely on other side of Y = 0 plane
  }
  if (a.y > HEIGHT && b.y > HEIGHT && c.y > HEIGHT) {
    return;  // entirely on other side of Y = HEIGHT plane
  }
  triangles.push_back(Triangle{
      Vertex{.x = a.x, .y = a.y, .z = a.z, .color = state.fillColor, .u = u1, .v = v1},
      Vertex{.x = b.x, .y = b.y, .z = b.z, .color = state.fillColor, .u = u2, .v = v2},
      Vertex{.x = c.x, .y = c.y, .z = c.z, .color = state.fillColor, .u = u3, .v = v3},
  });
}

inline void addRectangle(const Vector &a, const Vector &b, const Vector &c, const Vector &d) {
  addTriangle(a, b, c, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0);
  addTriangle(d, a, c, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0);
}

inline void addBox() {
  addRectangle(Vector{0.5, 0.5, 0.5, 1.0},    //
               Vector{-0.5, 0.5, 0.5, 1.0},   //
               Vector{-0.5, -0.5, 0.5, 1.0},  //
               Vector{0.5, -0.5, 0.5, 1.0});
  addRectangle(Vector{0.5, 0.5, -0.5, 1.0},    //
               Vector{0.5, -0.5, -0.5, 1.0},   //
               Vector{-0.5, -0.5, -0.5, 1.0},  //
               Vector{-0.5, 0.5, -0.5, 1.0});
  addRectangle(Vector{0.5, 0.5, 0.5, 1.0},    //
               Vector{0.5, 0.5, -0.5, 1.0},   //
               Vector{0.5, -0.5, -0.5, 1.0},  //
               Vector{0.5, -0.5, 0.5, 1.0});
  addRectangle(Vector{-0.5, 0.5, 0.5, 1.0},    //
               Vector{-0.5, 0.5, -0.5, 1.0},   //
               Vector{-0.5, -0.5, -0.5, 1.0},  //
               Vector{-0.5, -0.5, 0.5, 1.0});
  addRectangle(Vector{0.5, 0.5, 0.5, 1.0},    //
               Vector{0.5, 0.5, -0.5, 1.0},   //
               Vector{-0.5, 0.5, -0.5, 1.0},  //
               Vector{-0.5, 0.5, 0.5, 1.0});
  addRectangle(Vector{0.5, -0.5, 0.5, 1.0},    //
               Vector{0.5, -0.5, -0.5, 1.0},   //
               Vector{-0.5, -0.5, -0.5, 1.0},  //
               Vector{-0.5, -0.5, 0.5, 1.0});
}

inline std::size_t flush() {
  for (auto &triangle : triangles) {
    // Sort the vertices in each triangle, with (z, y, x)
    // The 'z' coordinate is most important to ensure proper drawing order
    std::sort(  //
        triangle.vertices, triangle.vertices + 3, [](const Vertex &lhs, const Vertex &rhs) -> bool {
          return lhs.z < rhs.z ||
                 (lhs.z == rhs.z && (lhs.y < rhs.y || (lhs.y == rhs.y && lhs.x < rhs.x)));
        });
  }
  // Sort the triangles.
  //
  // We sort primarily by z coordinates, but if there are ties, we also look
  // at y and x coordinates to reduce Z-fighting.
  //
  // This isn't always correct, but it's fast and simple and correct enough of the time.
  //
  std::sort(  //
      triangles.begin(), triangles.end(), [](const Triangle &lhs, const Triangle &rhs) -> bool {
        for (int i = 0; i < 3; i++) {
          if (lhs.vertices[i].z != rhs.vertices[i].z) {
            return lhs.vertices[i].z < rhs.vertices[i].z;
          }
        }
        for (int i = 0; i < 3; i++) {
          if (lhs.vertices[i].y != rhs.vertices[i].y) {
            return lhs.vertices[i].y < rhs.vertices[i].y;
          }
        }
        for (int i = 0; i < 3; i++) {
          if (lhs.vertices[i].x != rhs.vertices[i].x) {
            return lhs.vertices[i].x < rhs.vertices[i].x;
          }
        }
        return false;
      });
  auto &firstVertex = triangles.front().vertices[0];
  auto status = SDL_RenderGeometryRaw(  //
      renderer,
      boxTexture,                          // texture
      &firstVertex.x, sizeof(Vertex),      // xy coordinates
      &firstVertex.color, sizeof(Vertex),  // colors
      &firstVertex.u, sizeof(Vertex),      // uv coordinates
      3 * triangles.size(),                // number of vertices
      nullptr, 0, 0);                      // indices
  if (status != 0) {
    sdlError("SDL_RenderGeometryRaw");
  }
  auto triangleCount = triangles.size();
  triangles.clear();
  return triangleCount;
}

// Returns a random float in the range [low, high]
inline float randomUniform(float low, float high) {
  return low + (rand() / float(RAND_MAX)) * (high - low);
}

// Returns a random integer in the range [low, high]
// NOTE: inclusive of both low and high
inline int randomInt(int low, int high) { return int(randomUniform(low, high + 0.9999999)); }

inline SDL_Color randomColor() {
  return COLORS[randomInt(0, sizeof(COLORS) / sizeof(COLORS[0]) - 1)];
}

struct Box final {
  Vector position;
  Vector velocity;
  Vector rotation;
  Vector rotationalVelocity;
  Color color;
};

constexpr const float GRAVITY = 1.0;
constexpr const Vector CENTER = Vector{0, 0, -25, 1};

int main() {
  init(WIDTH, HEIGHT);

  // Instead of having coordinates on the screen be in [0, WIDTH] x [0, HEIGHT],
  // We can now think in terms of [-1, 1] x [-1, 1].
  // The downside being that when WIDTH != HEIGHT, the image will look a bit
  // stretched or squashed.
  apply(viewport());

  const Matrix PERSPECTIVE_MATRIX = perspective(  //
      TAU / 6,                                    // 60 degrees field of view
      WIDTH / float(HEIGHT),                      // aspect ratio
      1.0 / 1024,                                 // near
      1024);

  std::vector<Box> boxes;

  auto makeRandomBox = [&]() {
    boxes.push_back(Box{
        .position = CENTER + Vector{randomUniform(-10, 10), randomUniform(-10, 10),
                                    randomUniform(-10, 10), 0},
        .velocity = Vector{randomUniform(-0.1, 0.1), randomUniform(-0.1, 0.1),
                           randomUniform(-0.1, 0.1), 0.0},
        .rotation = Vector{0, 0, 0, 1},
        .rotationalVelocity = Vector{randomUniform(-0.1, 0.1), randomUniform(-0.1, 0.1),
                                     randomUniform(-0.1, 0.1), 0.0},
        .color = randomColor(),
    });
  };

  makeRandomBox();

  // auto keyboardState = SDL_GetKeyboardState(nullptr);
  auto programStartTime = SDL_GetTicks64();
  float totalFPS = 0.0f;
  float latestFPS = 0.0f;
  bool perspectiveEnabled = true;
  Uint64 lastLatestFPSComputeTime = programStartTime;
  Uint64 lastLatestFPSComputeFrame = 0;
  Uint64 lastRandomBoxFrame = 0;
  Uint64 framesPerNewRandomBox = FPS / 3.0;
  Uint64 busyMS = 0;

  auto jerk = [&](const Vector &v) {
    for (auto &box : boxes) {
      box.velocity += v;
    }
  };

  for (Uint64 frameCount = 0;; ++frameCount) {
    auto frameStartTime = SDL_GetTicks64();
    SDL_Event event;
    auto perSec = frameCount * SEC_PER_FRAME;
    (void)perSec;
    charX = charWidth;
    charY = charHeight / 2;

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          return 0;
        case SDL_KEYDOWN:
          switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_A:
              jerk(Vector{-0.1, 0, 0, 0});
              break;
            case SDL_SCANCODE_D:
              jerk(Vector{0.1, 0, 0, 0});
              break;
            case SDL_SCANCODE_W:
              jerk(Vector{0, 0.1, 0, 0});
              break;
            case SDL_SCANCODE_S:
              jerk(Vector{0, -0.1, 0, 0});
              break;
            case SDL_SCANCODE_P:
              perspectiveEnabled = !perspectiveEnabled;
              break;
            default:
              break;
          }
        default:
          break;
      }
    }

    if (boxes.size() < 200 && frameCount - lastRandomBoxFrame >= framesPerNewRandomBox) {
      lastRandomBoxFrame = frameCount;
      makeRandomBox();
    }
    for (auto &box : boxes) {
      box.velocity += normalize(CENTER - box.position, GRAVITY * SEC_PER_FRAME);
      box.position += box.velocity;
      box.rotation += box.rotationalVelocity;
    }

    clear(DARK_GREY);

    state.fillColor = PEACH;

    push();

    if (perspectiveEnabled) {
      apply(PERSPECTIVE_MATRIX);
    }

    for (const auto &box : boxes) {
      push();
      state.fillColor = box.color;
      apply(translation(box.position));
      apply(rotation(box.rotation));
      addBox();
      pop();
    }

    pop();

    auto triangleCount = flush();

    print("box count    = ", boxes.size(), "\n");
    print("frameCount   = ", frameCount, "\n");
    print("latestFPS    = ", latestFPS, "\n");
    print("totalFPS     = ", totalFPS, "\n");
    print("busyRatio    = ", busyMS / float(MS_PER_FRAME * frameCount), "\n");
    print("render count = ", triangleCount, "\n");

    charY = charHeight / 2;
    charX = WIDTH / 2;
    print("PERSPECTIVE ", (perspectiveEnabled ? "ON" : "OFF"));

    SDL_RenderPresent(renderer);
    auto frameEndTime = SDL_GetTicks64();
    totalFPS = frameCount / float(frameEndTime - programStartTime) * 1000;
    if (frameEndTime - lastLatestFPSComputeTime >= 1000) {
      latestFPS = (frameCount - lastLatestFPSComputeFrame) /
                  float(frameEndTime - lastLatestFPSComputeTime) * 1000;
      lastLatestFPSComputeFrame = frameCount;
      lastLatestFPSComputeTime = frameEndTime;
    }
    auto frameDuration = frameEndTime - frameStartTime;
    busyMS += frameDuration;
    if (frameDuration + 1 < MS_PER_FRAME) {
      SDL_Delay(MS_PER_FRAME - frameDuration);
    } else if (frameDuration > MS_PER_FRAME) {
      std::cerr << "WARNING: LATE FRAME: id = " << frameCount << " duration = " << frameDuration
                << "ms, MS_PER_FRAME = " << MS_PER_FRAME << std::endl;
    }
  }

  return 0;
}
