#include "crdt/graphsync_dagsyncer.hpp"

#include <ipfs_lite/ipld/impl/ipld_node_impl.hpp>

namespace sgns::crdt
{
GraphsyncDAGSyncer::GraphsyncDAGSyncer(const std::shared_ptr<IpfsDatastore>& service,
    const std::shared_ptr<Graphsync>& graphsync, const std::shared_ptr<libp2p::Host>& host)
    : dagService_(service)
    , graphsync_(graphsync)
    , host_(host)
{
}

outcome::result<void> GraphsyncDAGSyncer::Listen(const Multiaddress& listen_to)
{
    if (this->host_ == nullptr)
    {
        return outcome::failure(boost::system::error_code{});
    }

    auto listen_res = host_->listen(listen_to);
    if (listen_res.has_failure())
    {
        /*logger->trace("Cannot listen to multiaddress {}, {}",
            listen_to.getStringAddress(),
            listen_res.error().message());*/
        return listen_res.error();
    }
    auto startResult = this->StartSync();
    if (startResult.has_failure())
    {
        return startResult.error();
    }

    return outcome::success();
  }

  outcome::result<void> GraphsyncDAGSyncer::RequestNode(const PeerId& peer,  boost::optional<Multiaddress> address,
    const CID& root_cid)
  {
    auto startResult = this->StartSync();
    if (startResult.has_failure())
    {
      return startResult.error();
    }

    if (this->graphsync_ == nullptr)
    {
      return outcome::failure(boost::system::error_code{});
    }
    std::vector<Extension> extensions;
    ResponseMetadata response_metadata{};
    Extension response_metadata_extension = ipfs_lite::ipfs::graphsync::encodeResponseMetadata(response_metadata);
    extensions.push_back(response_metadata_extension);
    
    std::vector<CID> cids;
    Extension do_not_send_cids_extension = ipfs_lite::ipfs::graphsync::encodeDontSendCids(cids);
    extensions.push_back(do_not_send_cids_extension);
    auto subscription = this->graphsync_->makeRequest(peer, std::move(address),  root_cid, {}, extensions, 
        std::bind(&GraphsyncDAGSyncer::RequestProgressCallback, this, std::placeholders::_1, std::placeholders::_2));

    // keeping subscriptions alive, otherwise they cancel themselves
    this->requests_.push_back(std::shared_ptr<Subscription>(new Subscription(std::move(subscription))));

    return outcome::success();
}

outcome::result<void> GraphsyncDAGSyncer::addNode(std::shared_ptr<const ipfs_lite::ipld::IPLDNode> node)
{
    return dagService_.addNode(std::move(node));
}

outcome::result<std::shared_ptr<ipfs_lite::ipld::IPLDNode>> GraphsyncDAGSyncer::getNode(const CID& cid) const
{
    auto node = dagService_.getNode(cid);
    return node;
}

outcome::result<void> GraphsyncDAGSyncer::removeNode(const CID& cid)
{
    return dagService_.removeNode(cid);
}

outcome::result<size_t> GraphsyncDAGSyncer::select(
    gsl::span<const uint8_t> root_cid,
    gsl::span<const uint8_t> selector,
    std::function<bool(std::shared_ptr<const ipfs_lite::ipld::IPLDNode> node)> handler) const
{
    return dagService_.select(root_cid, selector, handler);
}

outcome::result<std::shared_ptr<ipfs_lite::ipfs::merkledag::Leaf>> GraphsyncDAGSyncer::fetchGraph(const CID& cid) const
{
    return dagService_.fetchGraph(cid);
}

outcome::result<std::shared_ptr<ipfs_lite::ipfs::merkledag::Leaf>> GraphsyncDAGSyncer::fetchGraphOnDepth(
    const CID& cid, uint64_t depth) const
{
    return dagService_.fetchGraphOnDepth(cid, depth);
}

outcome::result<bool> GraphsyncDAGSyncer::HasBlock(const CID& cid) const
{
    auto getNodeResult = dagService_.getNode(cid);
    return getNodeResult.has_value();
}

outcome::result<bool> GraphsyncDAGSyncer::StartSync()
{
    if (!started_)
    {
        if (graphsync_ == nullptr)
        {
            return outcome::failure(boost::system::error_code{});
        }

        auto dagService = std::make_shared<MerkleDagBridgeImpl>(shared_from_this());
        if (dagService == nullptr)
        {
            return outcome::failure(boost::system::error_code{});
        }

        BlockCallback blockCallback = std::bind(
            &GraphsyncDAGSyncer::BlockReceivedCallback, this, std::placeholders::_1, std::placeholders::_2);
        graphsync_->start(dagService, blockCallback);

        if (host_ == nullptr)
        {
            return outcome::failure(boost::system::error_code{});
        }
        host_->start();

        started_ = true;
    }
    return started_;
}

void GraphsyncDAGSyncer::StopSync()
{
    if (graphsync_ != nullptr)
    {
        graphsync_->stop();
    }
    if (host_ != nullptr)
    {
        host_->stop();
    }
    started_ = false;
}

outcome::result<GraphsyncDAGSyncer::PeerId> GraphsyncDAGSyncer::GetId() const
{
    if (host_ != nullptr)
    {
        return host_->getId();
    }
    return outcome::failure(boost::system::error_code{});
}

namespace
{
    std::string formatExtensions(const std::vector<GraphsyncDAGSyncer::Extension>& extensions)
    {
        std::string s;
        for (const auto& item : extensions) {
            s += fmt::format(
                "({}: 0x{}) ", item.name, common::Buffer(item.data).toHex());
        }
        return s;
    };
}

void GraphsyncDAGSyncer::RequestProgressCallback(
    ResponseStatusCode code, const std::vector<Extension>& extensions)
{
    logger_->trace("request progress: code={}, extensions={}", statusCodeToString(code), formatExtensions(extensions));
}

void GraphsyncDAGSyncer::BlockReceivedCallback(CID cid, sgns::common::Buffer buffer)
{
    logger_->trace("Block received: cid={}, extensions={}", cid.toString().value(), buffer.toHex());
    auto hb = HasBlock(cid);
    if (hb.has_value() && !hb.value())
    {
        auto node = ipfs_lite::ipld::IPLDNodeImpl::createFromRawBytes(buffer);
        if (!node.has_failure())
        {
            addNode(node.value());
        }
        else
        {
            logger_->error("Cannot create node from received block data for CID {}", cid.toString().value());
        }
    }
}
}
