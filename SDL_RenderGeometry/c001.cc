// part 002: Batch render triangles
//
// The result is exactly the same as part 001, but
// Now the triangle is not rendered until `flush()` is called.
//

#include <SDL2/SDL.h>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

[[noreturn]] void panic(const std::string_view &message) {
  std::cerr << "PANIC: " << message << std::endl;
  std::exit(1);
}

[[noreturn]] void sdlError(const std::string_view &tag) {
  std::cerr << "ERROR " << tag << ": " << SDL_GetError() << std::endl;
  std::exit(1);
}

static constexpr const bool DEBUG = true;
static constexpr const int WIDTH = 800;
static constexpr const int HEIGHT = 600;
static constexpr const double FPS = 60.0;
static constexpr const double SEC_PER_FRAME = 1 / FPS;
static constexpr const Uint64 MS_PER_FRAME = Uint64(1000 * SEC_PER_FRAME + 1);

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
constexpr const Color COLORS[] = {
    BLACK,       DARK_BLUE,   DARK_PURPLE,    DARK_GREEN,  BROWN,         DARK_GREY,  LIGHT_GREY,
    WHITE,       RED,         ORANGE,         YELLOW,      GREEN,         BLUE,       LAVENDER,
    PINK,        LIGHT_PEACH, BROWNISH_BLACK, DARKER_BLUE, DARKER_PURPLE, BLUE_GREEN, DARK_BROWN,
    DARKER_GREY, MEDIUM_GREY, LIGHT_YELLOW,   DARK_RED,    DARK_ORANGE,   LIME_GREEN, MEDIUM_GREEN,
    TRUE_BLUE,   MAUVE,       DARK_PEACH,     PEACH,
};

static SDL_Window *window;
static SDL_Renderer *renderer;

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
}

inline void clear(Color color) {
  if (SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a) != 0) {
    sdlError("SDL_SetRenderDrawColor");
  }
  if (SDL_RenderClear(renderer) != 0) {
    sdlError("SDL_RenderClear");
  }
}

struct Vector final {
  float x, y, z, w;
};

struct Vertex final {
  float x, y, z;
  Color color;
};

struct Triangle final {
  Vertex vertices[3];
};

static_assert(sizeof(Vertex) == 4 * sizeof(float));
static_assert(offsetof(Vertex, x) == 0 * sizeof(float));
static_assert(offsetof(Vertex, y) == 1 * sizeof(float));
static_assert(offsetof(Vertex, z) == 2 * sizeof(float));
static_assert(offsetof(Vertex, color) == 3 * sizeof(float));
static_assert(sizeof(Triangle) == 3 * sizeof(Vertex));

static std::vector<Triangle> triangles;

inline void addTriangle(Color color, Vector a, Vector b, Vector c) {
  triangles.push_back(Triangle{
      Vertex{.x = a.x, .y = a.y, .z = a.z, .color = color},
      Vertex{.x = b.x, .y = b.y, .z = b.z, .color = color},
      Vertex{.x = c.x, .y = c.y, .z = c.z, .color = color},
  });
}

inline void flush() {
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
      nullptr,                             // texture
      &firstVertex.x, sizeof(Vertex),      // xy coordinates
      &firstVertex.color, sizeof(Vertex),  // colors
      nullptr, 0,                          // uv coordinates
      3 * triangles.size(),                // number of vertices
      nullptr, 0, 0);                      // indices
  if (status != 0) {
    sdlError("SDL_RenderGeometryRaw");
  }
  triangles.clear();
}

int main() {
  init(WIDTH, HEIGHT);

  for (;;) {
    auto frameStartTime = SDL_GetTicks64();
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          return 0;
      }
    }

    clear(DARK_GREY);

    addTriangle(  //
        PEACH,    //
        Vector{.x = WIDTH * 1.0f / 3.0f, .y = HEIGHT * 2.0f / 3, .w = 1.0},
        Vector{.x = WIDTH / 2.0f, .y = HEIGHT * 1.0f / 3, .w = 1.0},
        Vector{.x = WIDTH * 2.0f / 3.0f, .y = HEIGHT * 2.0f / 3, .w = 1.0});

    flush();
    SDL_RenderPresent(renderer);
    auto frameEndTime = SDL_GetTicks64();
    auto frameDuration = frameEndTime - frameStartTime;
    if (frameDuration + 1 < MS_PER_FRAME) {
      SDL_Delay(MS_PER_FRAME - frameDuration);
    }
  }

  return 0;
}