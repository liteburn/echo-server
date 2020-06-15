#include <cstdlib>
#include <boost/asio.hpp>
#include <iostream>
#include <boost/bind.hpp>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/buffer.hpp>

#define THREADS 10

using boost::asio::ip::tcp;

class connection : public boost::enable_shared_from_this<connection>, private boost::asio::noncopyable
{
    tcp::socket socket_;
    boost::array<char, 32> buffer_;
public:
    typedef boost::shared_ptr<connection> connection_ptr;
    explicit connection(boost::asio::io_context& io_context)
            : socket_(io_context)
    {
    }

    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    static connection_ptr create(boost::asio::io_context& io_context)
    {
        return connection_ptr (new connection(io_context));
    }
    void start()
    {
        boost::asio::async_read(socket_, boost::asio::buffer(buffer_), boost::asio::transfer_at_least(1),
                                boost::bind(&connection::handle_read, shared_from_this(),
                                                         _1, _2));
    }

private:
    void handle_read(const boost::system::error_code& e,
                     std::size_t bytes_transferred) {
        if (!e) {
                boost::asio::async_write(socket_, boost::asio::buffer(buffer_),
                                         boost::bind(&connection::handle_write, shared_from_this(),
                                                                  _1, _2));
        }
    }

    void handle_write(const boost::system::error_code& e, std::size_t bytes_transferred)
    {
        if (!e)
        {
            socket_.close();
        }
    }
};


class server : public boost::enable_shared_from_this<server>, private boost::asio::noncopyable
{
public:
    typedef boost::shared_ptr<server> server_ptr;
    server(const boost::asio::ip::tcp::endpoint &endpoint, size_t thread_pool_size)
            : thread_pool_size_(thread_pool_size),
              acceptor_(io_context_)

    {
        boost::system::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        acceptor_.bind(endpoint, ec);
        acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
        start_accept();
    }

    void run()
    {
        std::vector<boost::shared_ptr<std::thread> > threads;
        for (std::size_t i = 0; i < thread_pool_size_; ++i)
        {
            boost::shared_ptr<std::thread> thread(new std::thread(
                    boost::bind(&boost::asio::io_context::run, &io_context_)));
            threads.push_back(thread);
        }
        for (auto & thread : threads)
            thread->join();

    }

private:
    void start_accept()
    {
        connection::connection_ptr new_connection_ = connection::create(io_context_);
        acceptor_.async_accept(new_connection_->socket(),
                               boost::bind(&server::handle_accept, this, new_connection_,
                                           boost::asio::placeholders::error));
    }
    void handle_accept(const connection::connection_ptr& new_connection_, const boost::system::error_code& e)
    {
        if (!e)
        {
            new_connection_->start();
            connection::connection_ptr new_connection_2 = connection::create(io_context_);
            acceptor_.async_accept(new_connection_2->socket(),
                                   boost::bind(&server::handle_accept, this, new_connection_2,
                                               boost::asio::placeholders::error));
        }
    }

    std::size_t thread_pool_size_;
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
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


        server s(tcp::endpoint(tcp::v4(), std::atoi(argv[1])), boost::lexical_cast<std::size_t>(THREADS));
        s.run();


    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}