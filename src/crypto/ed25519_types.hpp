

#ifndef SUPERGENIUS_CORE_CRYPTO_ED25519_TYPES_HPP
#define SUPERGENIUS_CORE_CRYPTO_ED25519_TYPES_HPP

#include <ed25519/ed25519.h>
#include "base/blob.hpp"

namespace sgns::crypto {

  namespace constants::ed25519 {
    /**
     * Important constants to deal with ed25519
     */
    enum {
      PRIVKEY_SIZE = ed25519_privkey_SIZE,
      PUBKEY_SIZE = ed25519_pubkey_SIZE,
      SIGNATURE_SIZE = ed25519_signature_SIZE,
      SEED_SIZE = PRIVKEY_SIZE,
    };
  }  // namespace constants::ed25519

  using ED25519PrivateKey = base::Blob<constants::ed25519::PRIVKEY_SIZE>;
  using ED25519PublicKey = base::Blob<constants::ed25519::PUBKEY_SIZE>;

  struct ED25519Keypair {
    ED25519PrivateKey private_key;
    ED25519PublicKey public_key;

    bool operator==(const ED25519Keypair &other) const;
    bool operator!=(const ED25519Keypair &other) const;
  };

  using ED25519Signature = base::Blob<constants::ed25519::SIGNATURE_SIZE>;

  using ED25519Seed = base::Blob<constants::ed25519::SEED_SIZE>;
}  // namespace sgns::crypto

#endif  // SUPERGENIUS_CORE_CRYPTO_ED25519_TYPES_HPP
