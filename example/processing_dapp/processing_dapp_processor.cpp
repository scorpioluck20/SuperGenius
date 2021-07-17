#include <processing/processing_service.hpp>
#include <crdt/globaldb/keypair_file_storage.hpp>
#include <crdt/globaldb/globaldb.hpp>

#include <libp2p/multi/multibase_codec/multibase_codec_impl.hpp>

#include <boost/program_options.hpp>
#include <boost/format.hpp>

#include <iostream>

using namespace sgns::processing;

namespace
{
    class ProcessingCoreImpl : public ProcessingCore
    {
    public:
        ProcessingCoreImpl(size_t nSubtasks, size_t subTaskProcessingTime)
            : m_nSubtasks(nSubtasks)
            , m_subTaskProcessingTime(subTaskProcessingTime)
        {
        }

        void SplitTask(const SGProcessing::Task& task, SubTaskList& subTasks) override
        {
            for (size_t i = 0; i < m_nSubtasks; ++i)
            {
                auto subtask = std::make_unique<SGProcessing::SubTask>();
                subtask->set_ipfsblock(task.ipfs_block_id());
                subtask->set_results_channel((boost::format("%s_subtask_%d") % task.results_channel() % i).str());
                subTasks.push_back(std::move(subtask));
            }
        }

        void  ProcessSubTask(
            const SGProcessing::SubTask& subTask, SGProcessing::SubTaskResult& result,
            uint32_t initialHashCode) override 
        {
            std::cout << "SubTask processing started. " << subTask.results_channel() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(m_subTaskProcessingTime));
            std::cout << "SubTask processed. " << subTask.results_channel() << std::endl;
            result.set_ipfs_results_data_id((boost::format("%s_%s") % "RESULT_IPFS" % subTask.results_channel()).str());
        }
        
    private:
        size_t m_nSubtasks;
        size_t m_subTaskProcessingTime;
    };


    class ProcessingTaskQueueImpl : public ProcessingTaskQueue
    {
    public:
        ProcessingTaskQueueImpl(
            std::shared_ptr<sgns::crdt::GlobalDB> db)
            : m_db(db)
            , m_processingTimeout(std::chrono::seconds(10))
        {
        }

        bool GrabTask(SGProcessing::Task& task) override
        {
            m_logger->info("GRAB_TASK");

            auto queryTasks = m_db->QueryKeyValues("tasks");
            if (queryTasks.has_failure())
            {
                m_logger->info("Unable list tasks from CRDT datastore");
                return false;
            }

            std::set<std::string> lockedTasks;
            if (queryTasks.has_value())
            {
                m_logger->info("TASK_QUEUE_SIZE: {}", queryTasks.value().size());
                bool isTaskGrabbed = false;
                for (auto element : queryTasks.value())
                {
                    auto taskKey = m_db->KeyToString(element.first);
                    if (taskKey.has_value())
                    {
                        bool isTaskLocked = IsTaskLocked(taskKey.value());
                        m_logger->debug("TASK_QUEUE_ITEM: {}, LOCKED: {}", taskKey.value(), isTaskLocked);

                        if (!isTaskLocked)
                        {
                            if (task.ParseFromArray(element.second.data(), element.second.size()))
                            {
                                if (LockTask(taskKey.value()))
                                {
                                    m_logger->debug("TASK_LOCKED {}", taskKey.value());
                                    return true;
                                }
                            }
                        }
                        else
                        {
                            m_logger->debug("TASK_PREVIOUSLY_LOCKED {}", taskKey.value());
                            lockedTasks.insert(taskKey.value());
                        }
                    }
                    else
                    {
                        m_logger->debug("Undable to convert a key to string");
                    }
                }

                // No task was grabbed so far
                for (auto lockedTask : lockedTasks)
                {
                    if (MoveExpiredTaskLock(lockedTask, task))
                    {
                        return true;
                    }
                }

            }
            return false;
        }

        bool CompleteTask(const SGProcessing::TaskResult& taskResult) override
        {
            sgns::base::Buffer data;
            data.put(taskResult.SerializeAsString());

            auto transaction = m_db->BeginTransaction();
            transaction->AddToDelta(sgns::crdt::HierarchicalKey("task_results/" + taskResult.task_id()), data);
            transaction->RemoveFromDelta(sgns::crdt::HierarchicalKey("lock_" + taskResult.task_id()));
            transaction->RemoveFromDelta(sgns::crdt::HierarchicalKey(taskResult.task_id()));

            auto res = transaction->PublishDelta();
            return !res.has_failure();
        }

        bool IsTaskLocked(const std::string& taskKey)
        {
            auto lockData = m_db->Get(sgns::crdt::HierarchicalKey("lock_" + taskKey));
            return (!lockData.has_failure() && lockData.has_value());
        }

        bool LockTask(const std::string& taskKey)
        {
            auto timestamp = std::chrono::system_clock::now();

            SGProcessing::TaskLock lock;
            lock.set_task_id(taskKey);
            lock.set_lock_timestamp(timestamp.time_since_epoch().count());

            sgns::base::Buffer lockData;
            lockData.put(lock.SerializeAsString());

            auto res = m_db->Put(sgns::crdt::HierarchicalKey("lock_" + taskKey), lockData);
            return !res.has_failure();
        }

        bool MoveExpiredTaskLock(const std::string& taskKey, SGProcessing::Task& task)
        {
            auto timestamp = std::chrono::system_clock::now();

            auto lockData = m_db->Get(sgns::crdt::HierarchicalKey("lock_" + taskKey));
            if (!lockData.has_failure() && lockData.has_value())
            {
                // Check task expiration
                SGProcessing::TaskLock lock;
                if (lock.ParseFromArray(lockData.value().data(), lockData.value().size()))
                {
                    auto expirationTime =
                        std::chrono::system_clock::time_point(
                            std::chrono::system_clock::duration(lock.lock_timestamp())) + m_processingTimeout;
                    if (timestamp > expirationTime)
                    {
                        auto taskData = m_db->Get(taskKey);

                        if (!taskData.has_failure())
                        {
                            if (task.ParseFromArray(taskData.value().data(), taskData.value().size()))
                            {
                                if (LockTask(taskKey))
                                {
                                    return true;
                                }
                            }
                        }
                        else
                        {
                            m_logger->debug("Unable to find a task {}", taskKey);
                        }
                    }
                }
            }
            return false;
        }

    private:
        std::shared_ptr<sgns::crdt::GlobalDB> m_db;
        std::chrono::system_clock::duration m_processingTimeout;
        sgns::base::Logger m_logger = sgns::base::createLogger("ProcessingTaskQueueImpl");
    };

    // cmd line options
    struct Options 
    {
        size_t serviceIndex = 0;
        size_t subTaskProcessingTime = 0; // ms
        size_t roomSize = 0;
        size_t disconnect = 0;
        size_t nSubTasks = 5;
        size_t channelListRequestTimeout = 5000;
        // optional remote peer to connect to
        std::optional<std::string> remote;
    };

    boost::optional<Options> parseCommandLine(int argc, char** argv) {
        namespace po = boost::program_options;
        try 
        {
            Options o;
            std::string remote;

            po::options_description desc("processing service options");
            desc.add_options()("help,h", "print usage message")
                ("remote,r", po::value(&remote), "remote service multiaddress to connect to")
                ("processingtime,p", po::value(&o.subTaskProcessingTime), "subtask processing time (ms)")
                ("roomsize,s", po::value(&o.roomSize), "subtask processing time (ms)")
                ("disconnect,d", po::value(&o.disconnect), "disconnect after (ms)")
                ("nsubtasks,n", po::value(&o.nSubTasks), "number of subtasks that task is split to")
                ("channellisttimeout,t", po::value(&o.channelListRequestTimeout), "chnnel list request timeout (ms)")
                ("serviceindex,i", po::value(&o.serviceIndex), "index of the service in computational grid (has to be a unique value)");

            po::variables_map vm;
            po::store(parse_command_line(argc, argv, desc), vm);
            po::notify(vm);

            if (vm.count("help") != 0 || argc == 1) 
            {
                std::cerr << desc << "\n";
                return boost::none;
            }

            if (o.serviceIndex == 0) 
            {
                std::cerr << "Service index should be > 0\n";
                return boost::none;
            }

            if (o.subTaskProcessingTime == 0)
            {
                std::cerr << "SubTask processing time should be > 0\n";
                return boost::none;
            }

            if (o.roomSize == 0)
            {
                std::cerr << "Processing room size should be > 0\n";
                return boost::none;
            }

            if (!remote.empty())
            {
                o.remote = remote;
            }

            return o;
        }
        catch (const std::exception& e) 
        {
            std::cerr << e.what() << std::endl;
        }
        return boost::none;
    }
}

int main(int argc, char* argv[])
{
    auto options = parseCommandLine(argc, argv);
    if (!options)
    {
        return 1;
    }

    auto loggerPubSub = libp2p::common::createLogger("GossipPubSub");
    //loggerPubSub->set_level(spdlog::level::trace);

    auto loggerProcessingEngine = libp2p::common::createLogger("ProcessingEngine");
    loggerProcessingEngine->set_level(spdlog::level::trace);

    auto loggerProcessingService = libp2p::common::createLogger("ProcessingService");
    loggerProcessingService->set_level(spdlog::level::trace);

    auto loggerProcessingTaskQueue = libp2p::common::createLogger("ProcessingTaskQueueImpl");
    loggerProcessingTaskQueue->set_level(spdlog::level::debug);

    auto loggerProcessingSubTaskQueue = libp2p::common::createLogger("ProcessingSubTaskQueue");
    loggerProcessingSubTaskQueue->set_level(spdlog::level::debug);
    
    auto loggerGlobalDB = libp2p::common::createLogger("GlobalDB");
    loggerGlobalDB->set_level(spdlog::level::debug);

    auto loggerDAGSyncer = libp2p::common::createLogger("GraphsyncDAGSyncer");
    loggerDAGSyncer->set_level(spdlog::level::trace);

    auto loggerBroadcaster = libp2p::common::createLogger("PubSubBroadcasterExt");
    loggerBroadcaster->set_level(spdlog::level::debug);
    
    const std::string processingGridChannel = "GRID_CHANNEL_ID";

    auto pubsubKeyPath = (boost::format("CRDT.Datastore.TEST.%d/pubs_processor") % options->serviceIndex).str();
    auto pubs = std::make_shared<sgns::ipfs_pubsub::GossipPubSub>(
        sgns::crdt::KeyPairFileStorage(pubsubKeyPath).GetKeyPair().value());

    std::vector<std::string> pubsubBootstrapPeers;
    if (options->remote)
    {
        pubsubBootstrapPeers = std::move(std::vector({ *options->remote }));
    }
    pubs->Start(40001, pubsubBootstrapPeers);

    const size_t maximalNodesCount = 1;

    boost::asio::deadline_timer timerToDisconnect(*pubs->GetAsioContext());
    if (options->disconnect > 0)
    {
        timerToDisconnect.expires_from_now(boost::posix_time::milliseconds(options->disconnect));
        timerToDisconnect.async_wait([pubs, &timerToDisconnect](const boost::system::error_code& error)
        {
            timerToDisconnect.expires_at(boost::posix_time::pos_infin);
            pubs->Stop();
        });
    }

    auto io = std::make_shared<boost::asio::io_context>();
    auto globalDB = std::make_shared<sgns::crdt::GlobalDB>(
        io, 
        (boost::format("CRDT.Datastore.TEST.%d") %  options->serviceIndex).str(), 
        40010 + options->serviceIndex,
        std::make_shared<sgns::ipfs_pubsub::GossipPubSubTopic>(pubs, "CRDT.Datastore.TEST.Channel"));

    auto crdtOptions = sgns::crdt::CrdtOptions::DefaultOptions();
    auto initRes = globalDB->Init(crdtOptions);

    std::thread iothread([io]() { io->run(); });

    auto taskQueue = std::make_shared<ProcessingTaskQueueImpl>(globalDB);

    auto processingCore = std::make_shared<ProcessingCoreImpl>(options->nSubTasks, options->subTaskProcessingTime);
    ProcessingServiceImpl processingService(pubs, maximalNodesCount, options->roomSize, taskQueue, processingCore);

    processingService.Listen(processingGridChannel);
    processingService.SetChannelListRequestTimeout(boost::posix_time::milliseconds(options->channelListRequestTimeout));

    processingService.SendChannelListRequest();

    // Gracefully shutdown on signal
    boost::asio::signal_set signals(*pubs->GetAsioContext(), SIGINT, SIGTERM);
    signals.async_wait(
        [&pubs, &io](const boost::system::error_code&, int)
        {
            pubs->Stop();
            io->stop();
        });

    pubs->Wait();
    iothread.join();

    return 0;
}

