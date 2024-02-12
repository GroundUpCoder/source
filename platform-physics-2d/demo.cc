#include <SDL2/SDL.h>
#include <SDL2_ttf/SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

constexpr double FPS = 60.0;
constexpr double SEC_PER_FRAME = 1 / FPS;
constexpr Uint64 MS_PER_FRAME = Uint64(1000 * SEC_PER_FRAME + 1);

constexpr SDL_Color DARK_GREY = SDL_Color{95, 87, 79, 255};        // 5
constexpr SDL_Color LAVENDER = SDL_Color{131, 118, 156, 255};      // 13
constexpr SDL_Color PEACH = SDL_Color{255, 157, 129, 255};         // 143 / -1
constexpr SDL_Color LIGHT_YELLOW = SDL_Color{243, 239, 125, 255};  // 135 / -9

[[noreturn]] inline void panic(const std::string_view &message) {
  std::cerr << "PANIC: " << message << std::endl;
  std::abort();
}

[[noreturn]] void sdlError(const std::string_view &tag) {
  std::cerr << "ERROR " << tag << ": " << SDL_GetError() << std::endl;
  std::exit(1);
}

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
  if (TTF_Init() != 0) {
    sdlError("TTF_Init");
  }
}

inline void clear(SDL_Color c) {
  if (SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a) != 0) {
    sdlError("SDL_SetRenderDrawColor");
  }
  if (SDL_RenderClear(renderer) != 0) {
    sdlError("SDL_RenderClear");
  }
}

inline void rect(SDL_Color c, const SDL_FRect &rect) {
  if (SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a) != 0) {
    sdlError("SDL_SetRenderDrawColor");
  }
  if (SDL_RenderFillRectF(renderer, &rect) != 0) {
    sdlError("SDL_RenderFillRect");
  }
}

// Monosapced Font Sheet
class FontSheet final {
  static constexpr const char FONT_SHEET_TEXT[] =
      "                "   //  0 - 15
      "                "   // 16 - 31
      " !\"#$%&'()*+,-./"  // 32 - 47
      "0123456789:;<=>?"   // 48 - 63
      "@ABCDEFGHIJKLMNO"   // 64 - 79
      "PQRSTUVWXYZ[\\]^_"  // 80 - 95
      "`abcdefghijklmno"   // 96 - 111
      "pqrstuvwxyz{|}~ ";  // 112 - 127

  static constexpr const std::size_t FONT_SHEET_TEXT_LEN = sizeof(FONT_SHEET_TEXT) - 1;

  static_assert(FONT_SHEET_TEXT_LEN == 128);

  SDL_Texture *texture;
  int charWidth, charHeight;

  static constexpr void swap(FontSheet &a, FontSheet &b) {
    std::swap(a.texture, b.texture);
    std::swap(a.charWidth, b.charWidth);
    std::swap(a.charHeight, b.charHeight);
  }

 public:
  ~FontSheet() {
    if (texture) SDL_DestroyTexture(texture);
  }
  constexpr FontSheet() : texture(nullptr), charWidth(0), charHeight(0) {}
  FontSheet(const FontSheet &) = delete;
  constexpr FontSheet(FontSheet &&f) noexcept : FontSheet() { swap(*this, f); }
  FontSheet &operator=(const FontSheet &) = delete;
  constexpr FontSheet &operator=(FontSheet &&f) noexcept { return swap(*this, f), *this; }

  FontSheet(SDL_Renderer *renderer, const char *file, int ptsize) {
    auto font = TTF_OpenFont(file, ptsize);
    if (!font) {
      sdlError("TTF_OpenFont");
    }
    auto surface = TTF_RenderUTF8_Blended(font, FONT_SHEET_TEXT, SDL_Color{255, 255, 255, 255});
    if (!surface) {
      sdlError("TTF_RenderUTF8_Blended");
    }
    if (surface->w % FONT_SHEET_TEXT_LEN != 0) {
      panic("Monospace font sheet width is not divisible by the number of characters (width = " +
            std::to_string(surface->w) + ", " + std::to_string(FONT_SHEET_TEXT_LEN) + ")");
    }
    charHeight = surface->h;
    charWidth = surface->w / FONT_SHEET_TEXT_LEN;
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
      sdlError("SDL_CreateTextureFromSurface");
    }
  }

  constexpr int getCharWidth() const { return charWidth; }
  constexpr int getCharHeight() const { return charHeight; }
  constexpr SDL_Texture *getTexture() const { return texture; }

  void setColor(SDL_Color color) {
    if (SDL_SetTextureColorMod(texture, color.r, color.g, color.b) != 0) {
      sdlError("SDL_SetTextureColorMod");
    }
  }

  void printChar(SDL_Renderer *renderer, int x, int y, char ch) {
    auto src = SDL_Rect{charWidth * (int)((Uint8)ch >= 128 ? ' ' : ch), 0, charWidth, charHeight};
    auto dst = SDL_Rect{x, y, charWidth, charHeight};
    if (SDL_RenderCopy(renderer, texture, &src, &dst) != 0) {
      sdlError("SDL_RenderCopy (printChar)");
    }
  }

  void print(SDL_Renderer *renderer, int x, int y, const std::string_view &sv) {
    for (auto ch : sv) {
      printChar(renderer, x, y, ch);
      x += charWidth;
    }
  }
};

struct Vector {  // 2D vector
  float x, y;
};
constexpr inline Vector operator+(Vector a, Vector b) { return Vector{a.x + b.x, a.y + b.y}; }
constexpr inline Vector &operator+=(Vector &a, const Vector &b) {
  a.x += b.x;
  a.y += b.y;
  return a;
}
constexpr inline Vector &operator*=(Vector &a, float b) {
  a.x *= b;
  a.y *= b;
  return a;
}
constexpr inline Vector operator*(Vector a, float b) { return Vector{a.x * b, a.y * b}; }
constexpr inline float min(float a, float b) { return std::min(a, b); }
constexpr inline float max(float a, float b) { return std::max(a, b); }
constexpr inline Vector min(Vector a, Vector b) {
  return Vector{std::min(a.x, b.x), std::min(a.y, b.y)};
}
constexpr inline Vector max(Vector a, Vector b) {
  return Vector{std::max(a.x, b.x), std::max(a.y, b.y)};
}
inline std::ostream &operator<<(std::ostream &out, const Vector &v) {
  return out << "Vector{" << v.x << ", " << v.y << "}";
}

struct Interval {  // 1D axis aligned bounding box
  float min, max;
  constexpr bool operator==(const Interval &) const = default;
  constexpr float length() const { return std::max(max - min, 0.0f); }
  explicit constexpr operator bool() const { return min < max; }
};
constexpr inline Interval operator&(const Interval &a, const Interval &b) {
  return Interval{max(a.min, b.min), min(a.max, b.max)};
}
constexpr inline Interval operator|(const Interval &a, const Interval &b) {
  return Interval{min(a.min, b.min), max(a.max, b.max)};
}
constexpr inline Interval getCollision(const Interval &a, float va, const Interval &b, float vb) {
  // t1: when will a.max be the same as b.min?
  // a.max + va * t1 = b.min + vb * t1
  auto t1 = (b.min - a.max) / (va - vb);

  // t2: when will a.min be the same as b.max?
  // a.min + va * t2 = b.max + vb * t2
  auto t2 = (b.max - a.min) / (va - vb);

  return t1 < t2 ? Interval{t1, t2} : Interval{t2, t1};
}
inline std::ostream &operator<<(std::ostream &out, const Interval &i) {
  return out << "Interval{" << i.min << ", " << i.max << "}";
}

struct Box {  // 2D axis aligned bounding box
  Interval x, y;
  explicit constexpr operator bool() const { return x && y; }
  constexpr bool operator==(const Box &) const = default;
  constexpr float width() const { return x.length(); }
  constexpr float height() const { return y.length(); }
};
constexpr inline Box &operator+=(Box &box, const Vector &v) {
  box.x.min += v.x;
  box.x.max += v.x;
  box.y.min += v.y;
  box.y.max += v.y;
  return box;
}
constexpr inline Box boxAt(float x, float y, float w, float h) {
  return Box{{x - w / 2, x + w / 2}, {y - h / 2, y + h / 2}};
}
constexpr inline Box operator&(const Box &a, const Box &b) { return Box{a.x & b.x, a.y & b.y}; }
constexpr inline Box operator|(const Box &a, const Box &b) { return Box{a.x | b.x, a.y | b.y}; }
constexpr inline Interval getCollision(const Box &a, Vector va, const Box &b, Vector vb) {
  return getCollision(a.x, va.x, b.x, vb.x) & getCollision(a.y, va.y, b.y, vb.y);
}
constexpr inline Interval getCollision(const Box &a, Vector va, const Box &b) {
  return getCollision(a, va, b, Vector{0, 0});
}
inline std::ostream &operator<<(std::ostream &out, const Box &box) {
  return out << "Box{{" << box.x.min << ", " << box.x.max << "}, {" << box.y.min << ", "
             << box.y.max << "}}";
}

struct Object {
  Box box;
  SDL_Color color;
};
inline void drawObject(const Object &object) {
  rect(object.color, {.x = object.box.x.min,
                      .y = object.box.y.min,
                      .w = object.box.width(),
                      .h = object.box.height()});
}

inline float randomUniform(float low, float high) {
  return low + (high - low) * float(double(rand()) / double(RAND_MAX));
}

constexpr float WIDTH = 600;
constexpr float HEIGHT = 400;
constexpr float PLAYER_WIDTH = 15;
constexpr float PLAYER_HEIGHT = 15;
constexpr float COIN_WIDTH = PLAYER_WIDTH * 0.75;
constexpr float COIN_HEIGHT = PLAYER_WIDTH * 0.75;
constexpr auto GRAVITY = Vector{0, +15};
constexpr float MOVE_SPEED = 5.0f;
constexpr float MAX_Y_VELOCITY = 15.0f;
constexpr auto COIN_COLOR = LIGHT_YELLOW;
constexpr auto PLAYER_COLOR = PEACH;
constexpr auto PLATFORM_COLOR = LAVENDER;

int main() {
  init(WIDTH, HEIGHT);
  auto font = FontSheet(renderer, "assets/RobotoMono.ttf", 24);
  const auto keyboard = SDL_GetKeyboardState(nullptr);

  auto player = Object{
      .box = boxAt(WIDTH / 2, HEIGHT / 2, PLAYER_WIDTH, PLAYER_HEIGHT),
      .color = PLAYER_COLOR,
  };
  auto playerVelocity = Vector{0, 0};
  auto coinCount = 0;

  std::vector<Object> platforms;
  platforms.push_back(Object{
      .box = boxAt(WIDTH / 2, HEIGHT - 40, WIDTH * 0.9, 10),
      .color = PLATFORM_COLOR,
  });
  platforms.push_back(Object{
      .box = boxAt(WIDTH / 2, 0, WIDTH * 1.5, 10),
      .color = PLATFORM_COLOR,
  });
  platforms.push_back(Object{
      .box = boxAt(WIDTH / 2, HEIGHT, WIDTH * 1.5, 10),
      .color = PLATFORM_COLOR,
  });
  platforms.push_back(Object{
      .box = boxAt(0, HEIGHT / 2, 10, HEIGHT * 1.5),
      .color = PLATFORM_COLOR,
  });
  platforms.push_back(Object{
      .box = boxAt(WIDTH, HEIGHT / 2, 10, HEIGHT * 1.5),
      .color = PLATFORM_COLOR,
  });
  platforms.push_back(Object{
      .box = boxAt(WIDTH / 4, HEIGHT * 3 / 4, WIDTH * 0.4, 10),
      .color = PLATFORM_COLOR,
  });
  platforms.push_back(Object{
      .box = boxAt(WIDTH / 8, HEIGHT / 2, 20, HEIGHT * 0.45),
      .color = PLATFORM_COLOR,
  });

  std::vector<Object> coins;
  for (int i = 0; i < 40; i++) {
    // make sure it does not intersect with a platform
    for (;;) {
    makeNewCoin:
      const auto x = randomUniform(2 * COIN_WIDTH, WIDTH - 2 * COIN_WIDTH);
      const auto y = randomUniform(2 * COIN_HEIGHT, HEIGHT - 2 * COIN_HEIGHT);
      const auto box = boxAt(x, y, COIN_WIDTH, COIN_HEIGHT);
      for (const auto platform : platforms) {
        if (box & platform.box) {
          goto makeNewCoin;  // intersection found, try again
        }
      }
      coins.push_back(Object{
          .box = box,
          .color = COIN_COLOR,
      });
      break;
    }
  }

  for (;;) {
    auto frameStartTime = SDL_GetTicks64();
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          return 0;
        case SDL_KEYDOWN:
          switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_SPACE:
              playerVelocity.y = min(0, playerVelocity.y) - 5;
              break;
            default:
              break;
          }
        default:
          break;
      }
    }
    if (keyboard[SDL_SCANCODE_A] || keyboard[SDL_SCANCODE_LEFT]) {
      playerVelocity.x = -MOVE_SPEED;
    }
    if (keyboard[SDL_SCANCODE_D] || keyboard[SDL_SCANCODE_RIGHT]) {
      playerVelocity.x = +MOVE_SPEED;
    }

    // update
    playerVelocity.x *= 0.95;
    playerVelocity += GRAVITY * SEC_PER_FRAME;
    playerVelocity.y = std::max(-MAX_Y_VELOCITY, std::min(MAX_Y_VELOCITY, playerVelocity.y));

    // collision with coins
    {
      std::size_t i = 0;
      while (i < coins.size()) {
        if (Interval{0, 1} & getCollision(player.box, playerVelocity, coins[i].box)) {
          coins.erase(coins.begin() + i);
          coinCount++;
        } else {
          i++;
        }
      }
    }

    // collision with platforms
    for (;;) {
      auto collisionTime = 1.0f;
      for (const auto &platform : platforms) {
        const auto collision =
            Interval{0, 1} & getCollision(player.box, playerVelocity, platform.box);
        if (collision) {
          collisionTime = std::min(collisionTime, collision.min);
        }
      }
      // If the only reason the player can't move is the y-axis, try again after
      // setting it to zero.
      if (collisionTime <= 0.0f && playerVelocity.y != 0) {
        playerVelocity.y = 0;
        continue;
      }
      player.box += playerVelocity * collisionTime;
      if (collisionTime < 1.0f) {
        playerVelocity = Vector{0, 0};
      }
      break;
    }

    // draw
    clear(DARK_GREY);
    font.print(renderer, font.getCharWidth(), font.getCharHeight() / 4,
               "COIN COUNT: " + std::to_string(coinCount));
    for (const auto &object : platforms) {
      drawObject(object);
    }
    for (const auto &coin : coins) {
      drawObject(coin);
    }
    drawObject(player);
    SDL_RenderPresent(renderer);

    auto frameEndTime = SDL_GetTicks64();
    auto frameDuration = frameEndTime - frameStartTime;
    if (frameDuration + 1 < MS_PER_FRAME) {
      SDL_Delay(MS_PER_FRAME - frameDuration);
    }
  }
}
