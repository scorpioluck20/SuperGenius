
#include "api/service/author/author_jrpc_processor.hpp"

#include "api/jrpc/jrpc_method.hpp"
#include "api/jrpc/value_converter.hpp"
#include "api/service/author/requests/pending_extrinsics.hpp"
#include "api/service/author/requests/submit_extrinsic.hpp"

namespace sgns::api::author {

  AuthorJRpcProcessor::AuthorJRpcProcessor(std::shared_ptr<JRpcServer> server,
                                           std::shared_ptr<AuthorApi> api)
      : api_{std::move(api)}, server_{std::move(server)} {
    BOOST_ASSERT(api_ != nullptr);
    BOOST_ASSERT(server_ != nullptr);
  }

  template <typename Request>
  using Handler = sgns::api::Method<Request, AuthorApi>;

  void AuthorJRpcProcessor::registerHandlers() {
    server_->registerHandler("author_submitExtrinsic",
                             Handler<request::SubmitExtrinsic>(api_));

    server_->registerHandler("author_pendingExtrinsics",
                             Handler<request::PendingExtrinsics>(api_));
  }

}  // namespace sgns::api::author
