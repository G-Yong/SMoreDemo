#ifndef SMOREDEMO_H
#define SMOREDEMO_H

#include <future>
#include <iostream>
#include <chrono>
#include <vector>
#include <string>

#include <opencv2/opencv.hpp>
#include "vimo_inference/vimo_inference.h"

using namespace smartmore;

int SMoreTest()
{
    try
    {
        /* =================== 参数区 =================== */
        std::string model_path = R"(C:/Users/Administrator/Desktop/vimoModel/vcloud/multiThread/model/model.vimosln)";
        std::string module_id = "2";
        bool use_gpu = true;
        int device_id = 0;

        std::string image_path = R"(C:/Users/Administrator/Desktop/vimoModel/vcloud/multiThread/1.jpg)";

        int thread_num = 2;          // 并发线程数
        constexpr int kRunTimes = 10;
        /* ============================================== */

        using Clock = std::chrono::steady_clock;
        using Ms = std::chrono::milliseconds;

        /* =================== 加载模型 =================== */
        vimo::Solution solution;
        solution.LoadFromFile(model_path);

        /* =================== 创建 pipelines =================== */
        std::vector<vimo::Pipelines> pipelines_list;
        pipelines_list.reserve(thread_num);

        for (int i = 0; i < thread_num; ++i)
        {
            pipelines_list.emplace_back(
                solution.CreatePipelines(module_id, use_gpu, device_id)
                );
        }

        /* =================== 创建 requests =================== */
        std::vector<vimo::Request> request_list;
        request_list.reserve(thread_num);

        for (int i = 0; i < thread_num; ++i)
        {
            cv::Mat img = cv::imread(image_path);
            if (img.empty())
                throw std::runtime_error("Failed to read image");

            request_list.emplace_back(img);
        }

        /* =================== worker =================== */
        auto worker = [](vimo::Pipelines& pipelines,
                         vimo::Request& req,
                         int idx) -> long long
        {
            vimo::Pipelines::UADResponseList rsps;

            auto start = Clock::now();
            pipelines.Run(req, rsps);
            auto end = Clock::now();

            auto cost =
                std::chrono::duration_cast<Ms>(end - start).count();

            return cost;
        };

        /* =================== benchmark =================== */
        for (int run = 0; run < kRunTimes; ++run)
        {
            std::cout << "\n========== Run " << run + 1 << " ==========\n";

            auto total_start = Clock::now();

            std::vector<std::future<long long>> futures;
            futures.reserve(thread_num);

            for (int i = 0; i < thread_num; ++i)
            {
                futures.emplace_back(
                    std::async(std::launch::async,
                               worker,
                               std::ref(pipelines_list[i]),
                               std::ref(request_list[i]),
                               i)
                    );
            }

            // 收集每个线程的耗时
            std::vector<long long> thread_costs(thread_num);
            for (int i = 0; i < thread_num; ++i)
            {
                thread_costs[i] = futures[i].get();
            }

            auto total_end = Clock::now();
            auto total_cost =
                std::chrono::duration_cast<Ms>(total_end - total_start).count();

            /* ======== 输出 ======== */
            for (int i = 0; i < thread_num; ++i)
            {
                std::cout << "Thread-" << i
                          << " cost: " << thread_costs[i] << " ms\n";
            }

            std::cout << "Total wall time: "
                      << total_cost << " ms\n";
        }

        std::cout << "\nDone." << std::endl;
    }
    catch (const vimo::VimoException& e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
#endif // SMOREDEMO_H
