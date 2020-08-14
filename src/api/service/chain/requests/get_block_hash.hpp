
#ifndef SUPERGENIUS_SRC_GET_BLOCK_HASH_HPP
#define SUPERGENIUS_SRC_GET_BLOCK_HASH_HPP

#include <jsonrpc-lean/request.h>

#include "api/service/chain/chain_api.hpp"
#include "primitives/extrinsic.hpp"

namespace sgns::api::chain::request {

  class GetBlockhash final {
    struct NoParameters {};
    using Param1 = primitives::BlockNumber;
    using Param2 = std::string;
    using VectorParam = boost::variant<Param1, Param2>;
    using Param3 = std::vector<VectorParam>;
    using Param = boost::variant<NoParameters, Param1, Param2, Param3>;

   public:
    explicit GetBlockhash(std::shared_ptr<ChainApi> api)
        : api_(std::move(api)){};

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    using ResultType = boost::variant<std::string, std::vector<std::string>>;
    outcome::result<ResultType> execute();

   private:
    std::shared_ptr<ChainApi> api_;
    Param param_;
  };

}  // namespace sgns::api::chain::request
#endif  // SUPERGENIUS_SRC_GET_BLOCK_HASH_HPP
