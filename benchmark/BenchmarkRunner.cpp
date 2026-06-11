#include "MatchingEngine.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>
#include <iomanip>

int main(int argc, char* argv[]) {
    size_t numOrders = 1000000;
    if (argc > 1) {
        try {
            numOrders = std::stoull(argv[1]);
        } catch (...) {
            std::cerr << "Invalid number of orders, defaulting to 1,000,000\n";
        }
    }

    std::cout << "==================================================\n";
    std::cout << "  MATCHING ENGINE LATENCY & THROUGHPUT BENCHMARK  \n";
    std::cout << "==================================================\n";
    std::cout << "Generating " << numOrders << " mock orders in memory...\n";

    // Pre-generate orders to prevent RNG from polluting latency measurements
    std::vector<Order> preGeneratedOrders;
    preGeneratedOrders.reserve(numOrders);

    std::default_random_engine generator(12345); // deterministic seed
    std::normal_distribution<double> priceDist(100.0, 5.0);
    std::uniform_int_distribution<uint32_t> qtyDist(10, 500);
    std::uniform_int_distribution<int> sideDist(0, 1);

    for (size_t i = 0; i < numOrders; ++i) {
        double price = std::round(priceDist(generator) * 100.0) / 100.0;
        if (price <= 0.0) price = 1.0;
        uint32_t qty = qtyDist(generator);
        Side side = (sideDist(generator) == 0) ? Side::BUY : Side::SELL;

        preGeneratedOrders.push_back(Order{
            static_cast<uint64_t>(i + 1),
            "TraderSim",
            side,
            price,
            qty,
            qty,
            0,
            OrderStatus::NEW
        });
    }

    std::cout << "Mock order generation complete. Starting matching benchmark...\n";

    MatchingEngine warmupEngine;
    std::vector<Trade> trades;
    trades.reserve(1000); // reuse buffer to avoid frequent re-allocations

    // Warm-up cache (10,000 orders)
    size_t warmupCount = std::min(numOrders, static_cast<size_t>(10000));
    for (size_t i = 0; i < warmupCount; ++i) {
        warmupEngine.processOrder(preGeneratedOrders[i], trades);
        trades.clear();
    }
    
    // Create a clean engine for the benchmark run
    MatchingEngine engine; 

    // Latency container in nanoseconds
    std::vector<double> latenciesUs;
    latenciesUs.reserve(numOrders);

    uint64_t totalTrades = 0;

    auto benchmarkStart = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < numOrders; ++i) {
        auto& order = preGeneratedOrders[i];

        auto start = std::chrono::high_resolution_clock::now();
        engine.processOrder(order, trades);
        auto end = std::chrono::high_resolution_clock::now();

        double elapsedUs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000.0;
        latenciesUs.push_back(elapsedUs);

        totalTrades += trades.size();
        trades.clear(); // clear buffer without deallocating capacity
    }

    auto benchmarkEnd = std::chrono::high_resolution_clock::now();
    double totalElapsedSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(benchmarkEnd - benchmarkStart).count() / 1000.0;

    // Analyze latencies
    std::sort(latenciesUs.begin(), latenciesUs.end());
    double sum = std::accumulate(latenciesUs.begin(), latenciesUs.end(), 0.0);
    double avg = sum / numOrders;

    double p50 = latenciesUs[static_cast<size_t>(numOrders * 0.50)];
    double p90 = latenciesUs[static_cast<size_t>(numOrders * 0.90)];
    double p95 = latenciesUs[static_cast<size_t>(numOrders * 0.95)];
    double p99 = latenciesUs[static_cast<size_t>(numOrders * 0.99)];
    double p999 = latenciesUs[static_cast<size_t>(numOrders * 0.999)];

    double throughput = numOrders / totalElapsedSeconds;

    std::cout << "\n------------------ RESULTS ------------------\n";
    std::cout << "Orders Processed: " << numOrders << "\n";
    std::cout << "Trades Generated: " << totalTrades << "\n";
    std::cout << "Total Time Taken: " << std::fixed << std::setprecision(3) << totalElapsedSeconds << " seconds\n";
    std::cout << "\nThroughput:       " << std::fixed << std::setprecision(0) << throughput << " orders/sec\n";
    std::cout << "\nLatencies (Microseconds):\n";
    std::cout << "  Average (Mean): " << std::fixed << std::setprecision(3) << avg << " us\n";
    std::cout << "  P50 (Median):   " << p50 << " us\n";
    std::cout << "  P90:            " << p90 << " us\n";
    std::cout << "  P95:            " << p95 << " us\n";
    std::cout << "  P99:            " << p99 << " us\n";
    std::cout << "  P99.9:          " << p999 << " us\n";
    std::cout << "---------------------------------------------\n";

    return 0;
}
