#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <sstream>
#include <fstream>
#include <memory>
   
struct metrics_t
{
    std::size_t blocks_count;
    std::size_t commands_count;
    std::size_t lines_count;
};

class bulk
{
    using system_time = std::chrono::time_point<std::chrono::system_clock>;
    using block       = std::vector<std::string>;
    using bulk_queue  = std::queue<std::shared_ptr<block>>;
    std::vector<std::thread> _hndl;
    block  subs;
    bulk_queue  log_queue;
    bulk_queue  file_queue;
    system_time sys_time;
    metrics_t   metrics;
public:
    explicit bulk(std::size_t _bulk_size,std::size_t _file_workers_count);
    ~bulk();
    void start();
private:
    void notify();
    auto run_bulk();
    auto get_dynamic_block();
    void file_worker    (const std::string& name);
    void terminal_worker(const std::string& name);
    void print_metrics  (const std::string& thread_name,const metrics_t& metrics,const bool& lines);
    std::size_t bulk_size;
    std::size_t time;
    std::atomic<bool> is_quit;
    std::condition_variable cv;
    std::mutex mute;
};
