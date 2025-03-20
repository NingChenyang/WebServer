// #include "Logger.h"
// #include <iostream>
// #include <thread>
// #include <string>
// #include <vector>
// #include <chrono>

// void LogInThread(int thread_id)
// {
//     for (int i = 0; i < 100; ++i)
//     {
//         LOG_DEBUG << "Debug info from thread " << thread_id;
//         LOG_INFO << "Info message from thread " << thread_id << " count " << i;
//         if (i % 10 == 0)
//         {
//             LOG_WARN << "Warning from thread " << thread_id << " count " << i;
//         }
//         if (i % 20 == 0)
//         {
//             LOG_ERROR << "Error from thread " << thread_id << " count " << i;
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//     }
// }

// void StressTest()
// {
//     // 创建多个线程进行压力测试
//     std::vector<std::thread> threads;
//     for (int i = 0; i < 4; ++i)
//     {
//         threads.emplace_back(LogInThread, i);
//     }

//     // 主线程也进行日志记录
//     for (int i = 0; i < 50; ++i)
//     {
//         LOG_INFO << "Main thread info message " << i;
//         if (i % 10 == 0)
//         {
//             LOG_WARN << "Main thread warning message " << i;
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }

//     // 等待所有线程完成
//     for (auto &t : threads)
//     {
//         if (t.joinable())
//         {
//             t.join();
//         }
//     }
// }

// int main()
// {
//     // 基本功能测试
//     LOG_INFO << "Starting log test...";
//     LOG_DEBUG << "This is a DEBUG message";
//     LOG_INFO << "This is an INFO message";
//     LOG_WARN << "This is a WARN message";
//     LOG_ERROR << "This is an ERROR message";

//     // 测试不同类型的数据
//     LOG_INFO << "Test different data types:";
//     LOG_INFO << "Integer: " << 42;
//     LOG_INFO << "Float: " << 3.14159;
//     LOG_INFO << "String: " << std::string("Hello World");
//     LOG_INFO << "Char: " << 'A';
//     LOG_INFO << "Bool: " << true;

//     // 压力测试
//     LOG_INFO << "Starting stress test...";
//     StressTest();
//     LOG_INFO << "Stress test completed.";

//     // 等待所有日志写入完成
//     std::this_thread::sleep_for(std::chrono::seconds(2));
//     return 0;
// }
