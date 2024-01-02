// Load a Bitmap file and dump some basic information about it.
//

#include <SDL2/SDL.h>

#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <utility>

static constexpr const bool VERBOSE = false;

[[noreturn]] void panic(const std::string_view &message) {
  std::cerr << "PANIC: " << message << std::endl;
  std::exit(1);
}

[[noreturn]] void sdlError(const std::string_view &tag) {
  std::cerr << "ERROR " << tag << ": " << SDL_GetError() << std::endl;
  std::exit(1);
}

inline auto operator<=>(const SDL_Color &lhs, const SDL_Color &rhs) {
  if (lhs.r != rhs.r) {
    return lhs.r <=> rhs.r;
  }
  if (lhs.g != rhs.g) {
    return lhs.g <=> rhs.g;
  }
  if (lhs.b != rhs.b) {
    return lhs.b <=> rhs.b;
  }
  return lhs.a <=> rhs.a;
}

inline void init() {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    sdlError("SDL_Init");
  }
}

static constexpr const int P1 = 'z' - 'a' + 1;
static constexpr const int P2 = 'Z' - 'A' + 1;
static constexpr const int P3 = '9' - '0' + 1;
char encodeHalfByte(Uint8 halfByte) {
  if (halfByte < 10) {
    return '0' + char(halfByte);
  }
  if (halfByte < 16) {
    return 'A' + char(halfByte - 10);
  }
  std::cerr << "halfByte = " << int(halfByte) << std::endl;
  panic("COULD NOT ENCODE BYTE");
}
void encode(Uint8 byte, std::string &out) {
  out.push_back(encodeHalfByte(byte / 16));
  out.push_back(encodeHalfByte(byte % 16));
}

int main(int argc, const char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: ./a.out <file>" << std::endl;
    std::exit(1);
  }
  init();

  auto surface = SDL_LoadBMP(argv[1]);

  if (!surface) {
    std::cerr << "Failed to load bitmap file " << argv[1] << std::endl;
    std::exit(1);
  }

  auto pixelFormatName = SDL_GetPixelFormatName(surface->format->format);

  std::cout << "BITMAP FILE " << argv[1] << std::endl;
  std::cout << "    PIXEL WIDTH           = " << surface->w << std::endl;
  std::cout << "    PIXEL HEIGHT          = " << surface->h << std::endl;
  std::cout << "    PIXEL WIDTH % 128     = " << (surface->w % 128) << std::endl;
  std::cout << "    PIXEL FORMAT          = " << pixelFormatName << std::endl;

  if (surface->format->BitsPerPixel == 32) {
    std::map<SDL_Color, std::size_t> colors;

    // The most basic custom RLE, just to see if I can make tis information compact.
    std::vector<std::pair<Uint8, Uint8>> rle;

    SDL_Color lastColor;
    bool variesOnlyInAlpha = true;

    for (int y = 0; y < surface->h; y++) {
      auto row = static_cast<void *>(static_cast<Uint8 *>(surface->pixels) + (y * surface->pitch));
      for (int x = 0; x < surface->w; x++) {
        auto color = static_cast<SDL_Color *>(row)[x];
        colors[color]++;
        if ((x || y) &&
            (color.r != lastColor.r && color.g != lastColor.g && color.b != lastColor.b)) {
          variesOnlyInAlpha = false;
        }
        if (rle.empty() || rle.back().first != color.a) {
          rle.push_back(std::make_pair<Uint8, Uint8>(Uint8(color.a), 1));
        } else {
          rle.back().second++;
        }
        lastColor = color;
      }
    }
    std::cout << "    " << colors.size() << " DISTINCT COLORS";
    if (variesOnlyInAlpha) {
      std::cout << " (varies only in alpha)";
    }
    if (VERBOSE) {
      std::cout << " (R, G, B, A)" << std::endl;
      for (const auto &pair : colors) {
        auto &color = pair.first;
        auto count = pair.second;
        std::cout << "        " << int(color.r) << ", " << int(color.g) << ", " << int(color.b)
                  << ", " << int(color.a) << " -> FOUND " << count << " TIMES" << std::endl;
      }
    } else {
      std::cout << std::endl;
    }

    if (variesOnlyInAlpha) {
      std::cout << "    ALPHA ONLY RLE LENGTH = " << (2 * rle.size()) << std::endl;
      if (VERBOSE) {
        // for (std::size_t i = 0; i < rle.size(); i++) {
        //   if (i % 8 == 0) {
        //     if (i) {
        //       std::cout << std::endl;
        //     }
        //     std::cout << "        ";
        //   }
        //   std::cout << "(" << int(rle[i].first) << ", " << int(rle[i].second) << "), ";
        // }
        std::string encoded;
        for (const auto &pair : rle) {
          encode(pair.first, encoded);
          encode(pair.second, encoded);
        }
        std::cout << "        ENCODED = " << encoded << std::endl;
      }
    }
  }

  return 0;
}
