#include <cstdlib>
#include <boost/asio.hpp>
#include <iostream>
#include <boost/bind.hpp>
#include <deque>
#include <boost/enable_shared_from_this.hpp>

#define THREADS 10

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
                                boost::bind(&session::done_read, shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
   }

    void done_read(const boost::system::error_code& error,
                     size_t bytes_transferred)
    {
        if (!error)
        {
            socket_.async_write_some(boost::asio::buffer(data_, max_length),
                                     boost::bind(&session::done_write, shared_from_this(),
                                                 boost::asio::placeholders::error));
        }
        else
        {
            std::cout << error.message() << std::endl;
            socket_.close();
        }

    }

    void done_write(const boost::system::error_code& error)
    {
        socket_.close();
    }

private:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};


class server: public boost::enable_shared_from_this<server>
{
public:
    server(boost::asio::io_service& io_service, short port)
            : io_service_(io_service),
              acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {
       start();
    }

    void handle_accept(const session::pointer& new_session,
                       const boost::system::error_code& error)
    {
        if (!error)
        {
            //std::cout << "connected\n";
            new_session->start();
        }
        start();
    }

    void start()
    {
        session::pointer new_session = session::create(io_service_);

        acceptor_.async_accept(new_session->socket(),
                               boost::bind(&server::handle_accept, this, new_session,
                                           boost::asio::placeholders::error));
    }

private:
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
    //tcp::socket socket_;
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

        boost::asio::io_service io_service{THREADS};
        server serv(io_service, std::atoi(argv[1]));
        //std::make_shared<server>(io_service, std::atoi(argv[1]))->start();
        //serv.start();
        std::vector<std::thread> workers;
        for (int i=0; i<THREADS; i++)
            workers.emplace_back([&io_service] {io_service.run();});
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