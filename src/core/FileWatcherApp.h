#pragma once

#include <concepts>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <thread>

#include "ConfigParser.h"
#include "EventProcessor.h"
#include "FileEvent.h"
#include "IFileProbe.h"
#include "IFolderWatcher.h"
#include "RuleEngine.h"
#include "SettleDetector.h"
#include "ThreadSafeQueue.h"

struct AppConfig {
    std::filesystem::path watchPath;
    std::filesystem::path configPath;
    std::chrono::milliseconds quietPeriod{500};
};

template <typename Watcher, typename Probe>
    requires std::derived_from<Watcher, IFolderWatcher> &&
             std::derived_from<Probe, IFileProbe>
class FileWatcherApp {
public:
    explicit FileWatcherApp(AppConfig config, Probe probe)
        : m_config(std::move(config))
        , m_watcher(m_config.watchPath)
        , m_probe(std::move(probe))
    {}

    ~FileWatcherApp() {
        if (!m_queue.is_closed()) stop();
    }

    bool init() {
        std::error_code ec;
        if (!std::filesystem::exists(m_config.watchPath, ec)) {
            std::cerr << "Watched directory does not exist: " << m_config.watchPath << '\n';
            return false;
        }

        const ConfigParser parser(m_config.configPath.string());
        auto parsedRules = parser.parse();
        if (!parsedRules) {
            std::cerr << "Failed to parse config: " << m_config.configPath << '\n';
            return false;
        }

        SettleDetector detector(m_config.quietPeriod, m_probe);
        RuleEngine ruleEngine(std::move(parsedRules.value()));

        m_processor.emplace(m_queue, std::move(detector), std::move(ruleEngine), m_config.watchPath);

        return true;
    }

    void start() {
        if (!m_processor) {
            std::cerr << "Cannot start app: not initialized.\n";
            return;
        }

        std::cout << "App Running.. "<< '\n'<< "Starting watcher on: " << m_watcher.getWatchedDir() << '\n';

        // Producer thread
        m_producerThread = std::jthread([this](const std::stop_token& token) {
            auto expect = m_watcher.start([this](FileEvent event) {
                if (!m_queue.push(std::move(event))) {
                    std::cerr << "Queue closed. Failed to push event.\n";
                }
            }, token);

            if (!expect) {
                std::cerr << "Watcher terminated with error: " << expect.error() << '\n';
            }

            // When watcher finishes, close the queue to release the consumer
            m_queue.close();
        });

        // Consumer thread
        m_consumerThread = std::jthread([this](const std::stop_token& token) {
            m_processor->run(token);
        });
    }

    void stop() {
        std::cout << "Stopping app...\n";
        m_producerThread.request_stop();
        m_consumerThread.request_stop();

        // Close queue to wake up consumer if it is waiting in pop()
        m_queue.close();

        // std::jthread members automatically join here when destroyed
    }

private:
    AppConfig m_config;
    ThreadSafeQueue<FileEvent> m_queue;
    Watcher m_watcher;
    Probe m_probe;
    std::optional<EventProcessor> m_processor;

    std::jthread m_producerThread;
    std::jthread m_consumerThread;
};