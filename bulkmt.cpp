#include <iostream>
#include "bulkmt.hpp"


int main(int argc , char* argv[])
{
        auto file_threads_count = 2;
        if (argc>1)
        {
            bulk b(std::atoi(argv[argc-1]),file_threads_count);
            b.start();
        }
        else
        {
            std::cerr << "bulk: no arguments to run handler" << std::endl;
        }
        
    return 0;
}