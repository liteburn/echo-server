#include <cstdlib>
#include <boost/asio.hpp>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <thread>

#define THREADS 16

using boost::asio::ip::tcp;

class session : public boost::enable_shared_from_this<session>
{
        tcp::socket socket_;
        boost::asio::streambuf data;
public:
    typedef boost::shared_ptr<session> pointer;
    session(tcp::socket socket)
            : socket_(std::move(socket))
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    static pointer create(tcp::socket socket)
    {
        return pointer(new session(std::move(socket)));
    }
    void run()
    {
        start();
    }

    void start()
    {
        async_read(socket_, data, boost::asio::transfer_at_least(1),
                                boost::bind(&session::done_read, shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }

    void done_read(const boost::system::error_code& error,
                   size_t bytes_transferred)
    {
        if (!error)
        {
            if (bytes_transferred!=0){
                async_write(socket_ , data,
                            boost::bind(&session::done_write, shared_from_this(),
                                        boost::asio::placeholders::error));
            } else {
                async_read(socket_, data, boost::asio::transfer_at_least(1),
                           boost::bind(&session::done_read, shared_from_this(),
                                       boost::asio::placeholders::error,
                                       boost::asio::placeholders::bytes_transferred));

            }
        }

    }

    void done_write(const boost::system::error_code& error)
    {
        socket_.close();
    }
};


class server: public boost::enable_shared_from_this<server>
{
public:
    typedef boost::shared_ptr<server> pointer;
    server(boost::asio::io_service& io_service, const boost::asio::ip::tcp::endpoint &endpoint)
            : io_service_(io_service),
              acceptor_(io_service, endpoint),
              socket_(io_service)
    {

    }

    static pointer create(boost::asio::io_service& io_service, const tcp::endpoint &endpoint)
    {
        return pointer(new server(io_service, endpoint));
    }

    void handle_accept(const boost::system::error_code& error)
    {
        if (!error)
        {
            session::pointer new_session = session::create(std::move(socket_));
            new_session->run();
        }

        start();
    }

    void start()
    {


        acceptor_.async_accept(socket_,
                               boost::bind(&server::handle_accept, shared_from_this(),
                                           boost::asio::placeholders::error));
    }

    void run()
    {
        start();
    }

private:
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
};


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
        server::pointer serv = server::create(std::ref(io_service), tcp::endpoint(tcp::v4(), std::atoi(argv[1])));
        serv->run();
        std::vector<std::thread> workers;
        for (int i = 0; i < THREADS; i++)
            workers.emplace_back([&io_service] {io_service.run();});
        for (auto& w:workers)
            w.join();

    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
