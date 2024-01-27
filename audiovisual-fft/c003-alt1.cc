// c003: Like 002, but instead of playing audio, listens on microphone.

#include <SDL2/SDL.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <complex>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <numbers>
#include <span>
#include <vector>

// Computes FFT in-place, radix-2 DIT
//
// References:
//   https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm
//   https://gist.github.com/lukicdarkoo/3f0d056e9244784f8b4a
template <class Scalar, std::size_t N>
  requires(std::floating_point<Scalar> && std::bit_ceil(N) == N)
void fastFourierTransform(std::complex<Scalar> *x) {
  if constexpr (N <= 1) {
    return;
  } else {
    std::complex<Scalar> odd[N / 2];
    std::complex<Scalar> even[N / 2];
    for (std::size_t i = 0; i < N / 2; i++) {
      even[i] = x[2 * i];
      odd[i] = x[2 * i + 1];
    }
    fastFourierTransform<Scalar, N / 2>(even);
    fastFourierTransform<Scalar, N / 2>(odd);
    for (std::size_t k = 0; k < N / 2; k++) {
      auto t = std::exp(std::complex<Scalar>(
                   0, -std::numbers::pi_v<Scalar> * 2 * Scalar(k) / Scalar(N))) *
               odd[k];
      x[k] = even[k] + t;
      x[N / 2 + k] = even[k] - t;
    }
  }
}

[[noreturn]] static void sdlError(const std::string_view &tag) {
  std::cerr << "ERROR " << tag << ": " << SDL_GetError() << std::endl;
  std::exit(1);
}

using Color = SDL_Color;
constexpr const Color DARK_GREY = Color{95, 87, 79, 255};  // 5
constexpr const Color ORANGE = Color{255, 163, 0, 255};    // 9
constexpr const Color GREEN = Color{0, 228, 54, 255};      // 11
constexpr const Color BLUE = Color{41, 173, 255, 255};     // 12

constexpr const int WIDTH = 800;
constexpr const int HEIGHT = 600;
constexpr const float FPS = 60.0;
constexpr const float SEC_PER_FRAME = 1 / FPS;
constexpr const Uint64 MS_PER_FRAME = Uint64(1000 * SEC_PER_FRAME + 1);
constexpr const int CHUNK_SIZE = 2048;
constexpr const int FREQUENCY = 48000;
constexpr const int DISPLAY_FREQ_BIN_COUNT = 128;

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_AudioDeviceID recordingDevice;

static void init(int w, int h) {
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

static void clear(Color color) {
  if (SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a) != 0) {
    sdlError("SDL_SetRenderDrawColor");
  }
  if (SDL_RenderClear(renderer) != 0) {
    sdlError("SDL_RenderClear");
  }
}

static void rect(Color color, const SDL_FRect &rect) {
  if (SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a) != 0) {
    sdlError("SDL_SetRenderDrawColor");
  }
  if (SDL_RenderFillRectF(renderer, &rect) != 0) {
    sdlError("SDL_RenderFillRect");
  }
}

int main() {
  init(WIDTH, HEIGHT);

  // Audio Device Metadata
  {
    SDL_AudioSpec spec;
    char *name;

    int n = SDL_GetNumAudioDevices(true);

    for (int i = 0; i < n; i++) {
      std::cout << "AUDIO DEVICE " << i << " -> " << SDL_GetAudioDeviceName(i, true) << std::endl;
    }

    if (SDL_GetDefaultAudioInfo(&name, &spec, true) != 0) {
      sdlError("SDL_GetDefaultAudioInfo");
    }

    std::cout << "DEFAULT RECORDING DEVICE NAME -> " << name << std::endl;
  }

  // Stream of Pulse-code modulation data
  auto pcm = std::vector<float>();
  {
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;

    desired.freq = FREQUENCY;
    desired.format = AUDIO_F32;
    desired.channels = 1;
    desired.samples = CHUNK_SIZE;
    desired.callback = [](void *userdata, Uint8 *stream, int len) {
      auto pcmPtr = static_cast<std::vector<float> *>(userdata);
      auto ptr = static_cast<float *>(static_cast<void *>(stream));
      auto count = len / sizeof(float);
      pcmPtr->insert(pcmPtr->end(), ptr, ptr + count);
    };
    desired.userdata = &pcm;

    std::cout << "CHOOSING AUDIO DEVICE " << SDL_GetAudioDeviceName(0, true) << std::endl;
    if ((recordingDevice = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0, true), true, &desired,
                                               &obtained, 0)) == 0) {
      sdlError("SDL_OpenAudioDevice");
    }
  }
  SDL_PauseAudioDevice(recordingDevice, false);

  // Buffer to store the latest results of the FFT
  auto buffer = std::vector<std::complex<float>>();

  for (Uint64 frameCount = 0;; ++frameCount) {
    (void)frameCount;
    auto frameStartTime = SDL_GetTicks64();
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          return 0;
      }
    }

    if (pcm.size() >= CHUNK_SIZE) {
      buffer.clear();
      // SDL_LockAudio();
      buffer.insert(buffer.end(), pcm.begin(), pcm.end());
      pcm.clear();
      // SDL_UnlockAudio();
      fastFourierTransform<float, CHUNK_SIZE>(buffer.data());
    }

    clear(BLUE);
    auto rectCount = std::min<std::size_t>(buffer.size(), DISPLAY_FREQ_BIN_COUNT);
    for (std::size_t i = 0; i < rectCount; i++) {
      const float w = WIDTH / float(rectCount);
      const auto x = i * w;
      const auto h = std::abs(buffer[i]);
      auto ratio = std::fmod(
          1.0f + std::atan2(buffer[i].real(), buffer[i].imag()) / (2 * std::numbers::pi_v<float>),
          1.0f);
      auto h2 = h * ratio;
      rect(ORANGE, {x, HEIGHT - h - HEIGHT / 8, w, h});
      rect(GREEN, {x, HEIGHT - h2 - HEIGHT / 8, w, h2});
    }

    SDL_RenderPresent(renderer);
    auto frameEndTime = SDL_GetTicks64();
    auto frameDuration = frameEndTime - frameStartTime;
    if (frameDuration + 1 < MS_PER_FRAME) {
      SDL_Delay(MS_PER_FRAME - frameDuration);
    }
  }

  return 0;
}
