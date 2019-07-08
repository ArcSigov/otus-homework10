
#include "bulkmt.hpp"


bulk::bulk(std::size_t _bulk_size,std::size_t _file_workers_count):
            metrics{0,0,0},
            bulk_size(_bulk_size), 
            time(0),
            is_quit(false)
{
        std::size_t i = 0;
        _hndl.emplace_back(&bulk::terminal_worker ,this, "log");
        while (i != _file_workers_count )
        {
            _hndl.emplace_back(&bulk::file_worker    ,this,"file"+std::to_string(i+1));
            i++;
        }
};

bulk::~bulk()
{
    notify();
}

auto bulk::run_bulk()
{  
    static auto protector = 0;
    auto dynamic = 0;
    for (unsigned int i = 0; i < bulk_size;i++)
    {
        std::string line;
        if (std::getline(std::cin,line))
        {
            metrics.lines_count++;
            if (line == "{")
            {
                dynamic = 1;
                break;
            }
            else if (line != "}")
            {
                if (i==0)
                {
                    time = std::chrono::duration_cast<std::chrono::seconds>(sys_time.time_since_epoch()).count();
                }
                metrics.commands_count++;
                subs.push_back(line);
            }
        }
        else
        {
            protector = 1;
            break;
        }
        
    }
    return std::make_tuple(dynamic,protector);
}

auto bulk::get_dynamic_block()
{
    auto line_protector = 1;
    static auto protector = 0;
    auto dynamic = 1;
    static auto first = 1;
    std::string line {'\n'};
    std::string output;
    while (1)
    {
        if(std::getline(std::cin,line))
        {
            metrics.lines_count++;
            if (line == "{")
            {
                line_protector ++;
            }
            else if (line == "}")
            {
                if (line_protector)
                    line_protector--;
                
                if (!line_protector)
                {
                    dynamic = 0;
                    first = 1;
                    break;
                }
            }
            else
            {
                if (first)
                {
                    time = std::chrono::duration_cast<std::chrono::seconds>(sys_time.time_since_epoch()).count();
                    first = 0;
                }
                subs.push_back(line);
                metrics.commands_count++;
            }       
        }
        else
        { 
            protector = 1;
            break;
        }
    }
    if (line_protector)
    {
        subs.clear();
    }
    return std::make_tuple(dynamic,protector);
}

void bulk::notify()
{
    for (auto& it : _hndl)
    {
        if (it.joinable())
            it.join();
    }
}

void bulk::start()
{
    auto protector = 0;
    auto dynamic = 0;
    while (!protector)
    {
        {
            std::lock_guard<std::mutex> lk(mute);
            sys_time = std::chrono::system_clock::now();
            subs.clear();
            if (dynamic)
            {
                std::tie(dynamic,protector) = get_dynamic_block();
            }
            else
            {
                std::tie(dynamic,protector)  = run_bulk();
            }
            if (subs.size())
            {
                metrics.blocks_count++;
                log_queue.push(std::make_shared<block>(subs));
                file_queue.push(std::make_shared<block>(subs));
            }            
        }
        cv.notify_all();
    }

    auto completed = false;
    while (!completed)
    {
        {
            std::lock_guard<std::mutex> lk(mute);
            if (file_queue.empty() && log_queue.empty())
            {
                is_quit = true;
                completed = true;
            }
        }
    }
    
    cv.notify_all();
    print_metrics("main",metrics,true);
}

void bulk::file_worker(const std::string& name)
{
    int i = 0;
    metrics_t current{0,0,0};
    while (!is_quit)
    {
        std::unique_lock<std::mutex> lk(mute);
        cv.wait(lk,[&]()
        {
            return (!file_queue.empty() || is_quit);
        });

        if (!file_queue.empty())
        {
            current.blocks_count++;
            auto a = file_queue.front();    
            std::stringstream ss;
            ss <<  "bulk" << std::to_string(time)  << "_" << name << "_" << "task_" << i << ".log";
            ++i;
            std::ofstream output(ss.str());

            for (auto it = a->cbegin() ; it !=a->cend();it++)
            {
                if (it != a->cbegin())
                    output << ", ";
                output << *it;
                current.commands_count++;
            }
            output<< std::endl;
            output.close();
            file_queue.pop();   
        }
        lk.unlock();
    }
    print_metrics(name,current,false);
}

void bulk::terminal_worker(const std::string& name)
{
    metrics_t current{0,0,0};
    while(!is_quit)
    {
        std::unique_lock<std::mutex> lk(mute);
        cv.wait(lk,[&]()
        {
            return (!log_queue.empty() || is_quit);
        });
        
        if (!log_queue.empty())
        {
            current.blocks_count++;
            auto a = log_queue.front();
            std::cout << "bulk: ";
            for (auto it = a->cbegin() ; it !=a->cend();it++)
            {
                if (it != a->cbegin())
                    std::cout << ", ";
                std::cout << *it;
                current.commands_count++;
            }
            std::cout << std::endl;
            log_queue.pop();
        }
        lk.unlock();
    }
    print_metrics(name,current,false);
}

void bulk::print_metrics(const std::string& thread_name, const metrics_t& metrics, const bool& lines)
{
        std::lock_guard<std::mutex> lk(mute);
        std::cout << thread_name << "_thread blocks:" << metrics.blocks_count << " commands: " << metrics.commands_count;
        if (lines)
        std::cout << " lines: " << metrics.lines_count;
        std::cout << std::endl;
}

