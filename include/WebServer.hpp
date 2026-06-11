#pragma once

#include "MatchingEngine.hpp"
#include "Statistics.hpp"
#include "RiskEngine.hpp"
#include "Portfolio.hpp"
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>

class WebServer {
public:
    WebServer(MatchingEngine& engine, Statistics& stats, RiskEngine& risk, Portfolio& portfolio);
    ~WebServer();

    // Start server in background thread on specified port
    void start(uint16_t port);
    
    // Stop the server
    void stop();

private:
    MatchingEngine& engine;
    Statistics& stats;
    RiskEngine& risk;
    Portfolio& portfolio;

    std::atomic<bool> running;
    std::thread serverThread;
    uint16_t serverPort;

    struct Impl;
    std::unique_ptr<Impl> impl;

    void setupRoutes();
    void setupEngineCallbacks();
};
