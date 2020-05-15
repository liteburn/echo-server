#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include <chrono>
#include <atomic>

inline std::chrono::high_resolution_clock::time_point get_current_time_fenced() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto res_time = std::chrono::high_resolution_clock::now();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return res_time;
}

template<class D>
inline long long to_us(const D &d) {
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
}

using boost::asio::ip::tcp;

enum { max_length = 1024 };

int main(int argc, char* argv[])
{   std::atomic<int> requests = 0;
    auto start = get_current_time_fenced();
    while (true) {
        if(to_us(get_current_time_fenced() - start) > 1000000){
            std::cout << requests;
            exit(0);
        }
        try {
            if (argc != 3) {
                std::cerr << "Usage: blocking_tcp_echo_client <host> <port>\n";
                return 1;
            }

            boost::asio::io_service io_service;

            tcp::resolver resolver(io_service);
            tcp::resolver::query query(tcp::v4(), argv[1], argv[2]);
            tcp::resolver::iterator iterator = resolver.resolve(query);

            tcp::socket s(io_service);
            s.connect(*iterator);

            //using namespace std; // For strlen.
            //std::cout << "Enter message: ";
            //char request[max_length];
            const char *request = "ok";
            //std::cin.getline(request, max_length);
            size_t request_length = std::strlen(request);
            boost::asio::write(s, boost::asio::buffer(request, request_length));

            char reply[max_length];
            size_t reply_length = boost::asio::read(s,
                                                    boost::asio::buffer(reply, request_length));
            std::cout << "Reply is: ";
            std::cout.write(reply, reply_length);
            std::cout << "\n";

            requests++;
            //std::string sClientIp = s.remote_endpoint().address().to_string();
            //std::cout << sClientIp;


        }
        catch (std::exception &e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }

    }
    return 0;
}