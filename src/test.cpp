// 并发测试 main.cpp
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "buttonrpc.hpp" //

void client_task(int id, int num_requests) {
    buttonrpc client;
    client.as_client("127.0.0.1", 5555);

    for (int i = 0; i < num_requests; ++i) {
        std::string key = "key_" + std::to_string(id) + "_" + std::to_string(i);
        std::string value = "val_" + std::to_string(i);
        std::string cmd = "set " + key + " " + value;

        auto res = client.call<std::string>("redis_command", cmd).val();

        // 可选：校验返回值
        if (res != "OK") {
            std::cerr << "Thread " << id << " failed at request " << i << ": " << res << std::endl;
        }
    }
}

int main() {
    int thread_num = 10000;           // 并发线程数
    int requests_per_thread = 100; // 每个线程发1个请求

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int i = 0; i < thread_num; ++i) {
        threads.emplace_back(client_task, i, requests_per_thread);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start).count();

    std::cout << "Total time: " << duration << " seconds" << std::endl;
    std::cout << "Throughput: " << (thread_num * requests_per_thread) / duration << " req/s" << std::endl;

    return 0;
}
