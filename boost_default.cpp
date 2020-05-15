#include <cstdlib>
#include <boost/asio.hpp>
#include <iostream>
#include <boost/bind.hpp>
#include <deque>
#include <boost/enable_shared_from_this.hpp>

using boost::asio::ip::tcp;


class session : public boost::enable_shared_from_this<session>
{
public:
    typedef boost::shared_ptr<session> pointer;
    session(boost::asio::io_service& io_service)
            : socket_(io_service)
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    static pointer create(boost::asio::io_service& io_service)
    {
        return pointer(new session(io_service));
    }

    void start()
    {

        socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                boost::bind(&session::handle_read, shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));

        socket_.async_write_some(boost::asio::buffer(data_, max_length),
                                 boost::bind(&session::handle_write, shared_from_this(),
                                             boost::asio::placeholders::error));
    }

    void handle_read(const boost::system::error_code& error,
                     size_t bytes_transferred)
    {
        if (!error)
        {
            //std::cout << "Have read the text: " << data_ << "\n";

        }
        else
        {
            std::cout << error.message() << std::endl;
            delete this;
        }

    }

    void handle_write(const boost::system::error_code& error)
    {
        if (!error)
        {
            //std::cout << "Have written the text: "  << data_ << "\n";
        }
        else
        {
            std::cout << error.message() << std::endl;
            delete this;
        }
    }

private:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};


template<class T>
class concurrent_queue {
private:
    mutable std::mutex mut;
    std::deque<T> queue;
    std::condition_variable cv;


public:
    concurrent_queue() = default;

    ~concurrent_queue() = default;

    void push(T data) {
        {
            std::unique_lock<std::mutex> lg(mut);
            queue.push_back(data);
        }
        cv.notify_one();
    }

    void pop(T& data) {
        {
            std::unique_lock<std::mutex> lg(mut);
            cv.wait(lg, [this]() { return queue.size() > 0;});
            data = queue.front();
            queue.pop_front();
        }
    }
};

class server
{
public:
    server(boost::asio::io_service& io_service, short port, concurrent_queue<session::pointer>& que)
            : io_service_(io_service),
              acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {
       start(que);
    }

    void handle_accept(session::pointer new_session,
                       const boost::system::error_code& error, concurrent_queue<session::pointer>& que)
    {
        if (!error)
        {
            //std::cout << "connected\n";
            //new_session->start();
            que.push(std::ref(new_session));
        }
        start(que);
    }

private:
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
    void start(concurrent_queue<session::pointer>& que)
    {
        session::pointer new_session = session::create(io_service_);

        acceptor_.async_accept(new_session->socket(),
                               boost::bind(&server::handle_accept, this, new_session,
                                           boost::asio::placeholders::error, std::ref(que)));
    }
};

[[noreturn]] void handle_session(concurrent_queue<session::pointer>& que){
    while (true){
        session::pointer get_session;
        que.pop(get_session);
        get_session->start();
    }
}

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }

        boost::asio::io_service io_service;
        concurrent_queue<session::pointer> que;

        server s(io_service, std::atoi(argv[1]), std::ref(que));
        std::vector<std::thread> workers;
        for (int i=0; i<8; i++)
            workers.emplace_back(handle_session, std::ref(que));
        io_service.run();
        for (auto& w:workers)
            w.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}