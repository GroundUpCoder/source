// c000: A program that just calculates the FFT for various values.

#include <bit>
#include <complex>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <numbers>
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

template <std::size_t N>
void run(std::vector<std::complex<float>> &values) {
  fastFourierTransform<float, N>(values.data());
  std::cout << "  => ";
  for (std::size_t i = 0; i < N; i++) {
    if (i > 0) {
      std::cout << ' ';
    }
    std::cout << values[i];
  }
  std::cout << std::endl;
}

int main(int argc, const char **argv) {
  std::vector<std::complex<float>> values;
  for (int i = 1; i < argc; i++) {
    values.push_back(std::stof(argv[i]));
  }
  switch (values.size()) {
    case 1:
      run<1>(values);
      break;
    case 2:
      run<2>(values);
      break;
    case 4:
      run<4>(values);
      break;
    case 8:
      run<8>(values);
      break;
    case 16:
      run<16>(values);
      break;
    case 32:
      run<32>(values);
      break;
    case 64:
      run<64>(values);
      break;
    case 128:
      run<128>(values);
      break;
    case 256:
      run<256>(values);
      break;
    case 512:
      run<512>(values);
      break;
    case 1024:
      run<1024>(values);
      break;
    default:
      std::cerr << "Number of elements must be a power of 2 less than or equal to 1024, but got "
                << values.size() << std::endl;
      return 1;
  }
  return 0;
}
