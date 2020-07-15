

#ifndef SUPERGENIUS_CORE_CRYPTO_RANDOM_GENERATOR_HPP
#define SUPERGENIUS_CORE_CRYPTO_RANDOM_GENERATOR_HPP

#include "libp2p/crypto/random_generator.hpp"

namespace sgns::crypto {
  using RandomGenerator = libp2p::crypto::random::RandomGenerator;
  using CSPRNG = libp2p::crypto::random::CSPRNG;
}  // namespace sgns::crypto

#endif  // SUPERGENIUS_CORE_CRYPTO_RANDOM_GENERATOR_HPP
