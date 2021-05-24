#include "processing_engine.hpp"

ProcessingEngine::ProcessingEngine(
    std::shared_ptr<sgns::ipfs_pubsub::GossipPubSub> gossipPubSub,
    std::string nodeId,
    std::shared_ptr<ProcessingCore> processingCore)
    : m_processingCore(processingCore)
    , m_nodeId(std::move(nodeId))
    , m_gossipPubSub(gossipPubSub)
{
}

void ProcessingEngine::StartQueueProcessing(std::shared_ptr<ProcessingSubTaskQueue> subTaskQueue)
{
    if (IsQueueProcessingStarted())
    {
        StopQueueProcessing();
    }

    m_subTaskQueue = subTaskQueue;
    auto queue = m_subTaskQueue->GetQueueSnapshot();
    if (queue->items_size() > 0)
    {
        for (size_t subTaskIdx = 0; subTaskIdx < queue->items_size(); ++subTaskIdx)
        {
            const auto& subTask = queue->items(subTaskIdx).subtask();
            AddResultChannel(subTask.results_channel());
        }

        m_subTaskQueue->GrabSubTask(std::bind(&ProcessingEngine::OnSubTaskGrabbed, this, std::placeholders::_1));
    }
}

void ProcessingEngine::StopQueueProcessing()
{
    m_subTaskQueue.reset();
}

bool ProcessingEngine::IsQueueProcessingStarted() const
{
    return (m_subTaskQueue.get() != nullptr);
}

void ProcessingEngine::OnSubTaskGrabbed(boost::optional<const SGProcessing::SubTask&> subTask)
{
    if (subTask)
    {
        m_logger->debug("[GRABBED]. ({}).", subTask->results_channel());
        ProcessSubTask(*subTask);
    }
}

void ProcessingEngine::ProcessSubTask(SGProcessing::SubTask subTask)
{
    m_logger->debug("[PROCESSING_STARTED]. ({}).", subTask.results_channel());
    auto resultChannel = m_resultChannels[subTask.results_channel()];
    std::thread thread(
        [subTask(std::move(subTask)), resultChannel, this]()
    {
        SGProcessing::SubTaskResult result;
        // @todo set initial hash code that depends on node id
        m_processingCore->ProcessSubTask(subTask, result, std::hash<std::string>{}(m_nodeId));
        m_logger->debug("[PROCESSED]. ({}).", subTask.results_channel());
        resultChannel->Publish(result.SerializeAsString());
        m_logger->debug("[RESULT_SENT]. ({}).", subTask.results_channel());

        // @todo Should a new subtask be grabbed once the perivious one is processed?
        m_subTaskQueue->GrabSubTask(std::bind(&ProcessingEngine::OnSubTaskGrabbed, this, std::placeholders::_1));
    });
    thread.detach();
}

const std::vector<std::tuple<std::string, SGProcessing::SubTaskResult>>& ProcessingEngine::GetResults() const
{
    return m_results;
}

void ProcessingEngine::OnResultChannelMessage(
    boost::optional<const sgns::ipfs_pubsub::GossipPubSub::Message&> message)
{
    if (message)
    {
        SGProcessing::SubTaskResult result;
        if (result.ParseFromArray(message->data.data(), static_cast<int>(message->data.size())))
        {           
            m_logger->debug("[RESULT_RECEIVED]. ({}).", result.ipfs_results_data_id());
            // Results accumulation
            m_results.push_back(std::make_tuple(
                message->topics[0], std::move(result)));
        }
    }
}

std::shared_ptr<sgns::ipfs_pubsub::GossipPubSubTopic> ProcessingEngine::AddResultChannel(
    const std::string& resultChannelId)
{
    using GossipPubSubTopic = sgns::ipfs_pubsub::GossipPubSubTopic;

    auto itChannel = m_resultChannels.find(resultChannelId);
    if (itChannel != m_resultChannels.end())
    {
        return itChannel->second;
    }

    auto resultChannel = std::make_shared<GossipPubSubTopic>(m_gossipPubSub, resultChannelId);
    resultChannel->Subscribe(std::bind(&ProcessingEngine::OnResultChannelMessage, this, std::placeholders::_1));
    m_resultChannels[resultChannelId] = resultChannel;
    return resultChannel;
}
