
#include "bulkmt.hpp"


bulk::bulk(std::size_t _bulk_size,std::size_t _file_workers_count):
            bulk_size(_bulk_size), 
            time(0),
            is_quit(false),
            data_is_logged(false),
            metrics{0,0,0}
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
    while (!protector || !q.empty())
    {
        {
            std::lock_guard<std::mutex> lk(mute);
            sys_time = std::chrono::system_clock::now();
            if (q.empty())
            {
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
                    q.push(subs);
                }
            }
        }
        cv.notify_all();
    }
    is_quit = true;
    print_metrics("main",metrics,true);
    cv.notify_all();
    
}

void bulk::file_worker(std::string name)
{
    int i = 0;
    metrics_t current{0,0,0};
    while (!is_quit)
    {
        std::unique_lock<std::mutex> lk(mute);
        cv.wait(lk,[&]()
        {
            return (!q.empty() || is_quit);
        });
        if (data_is_logged)
        {
            current.blocks_count++;
            auto a = q.front();    
            std::stringstream ss;
            ss <<  "bulk" << std::to_string(time)  << "_" << name << "_" << "task_" << i << ".log";
            ++i;
            std::ofstream output(ss.str());
            output << "bulk: ";

            for (auto it = a.cbegin() ; it !=a.cend();it++)
            {
                if (it != a.cbegin())
                    output << ", ";
                output << *it;
                current.commands_count++;
            }
            output<< std::endl;
            output.close();
            data_is_logged = false;
            q.pop();
            lk.unlock();
        }
    }
    print_metrics(name,current,false);
}

void bulk::terminal_worker(std::string name)
{
    metrics_t current{0,0,0};
    while(!is_quit)
    {
        std::unique_lock<std::mutex> lk(mute);
        cv.wait(lk,[&]()
        {
            return (!q.empty() || is_quit) && !data_is_logged;
        });
        
        if (!q.empty())
        {
            current.blocks_count++;
            auto a = q.front();
            std::cout << "bulk: ";
            for (auto it = a.cbegin() ; it !=a.cend();it++)
            {
                if (it != a.cbegin())
                    std::cout << ", ";
                std::cout << *it;
                current.commands_count++;
            }
            std::cout << std::endl;
            data_is_logged = true;
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

