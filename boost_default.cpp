#include <cstdlib>
#include <boost/asio.hpp>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/buffer.hpp>

#define THREADS 10

using boost::asio::ip::tcp;

class connection : public boost::enable_shared_from_this<connection>, private boost::asio::noncopyable
{
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    tcp::socket socket_;
    boost::array<char, 32> buffer_;
public:
    typedef boost::shared_ptr<connection> connection_ptr;
   explicit connection(boost::asio::io_context& io_context)
            : strand_(boost::asio::make_strand(io_context)),
              socket_(strand_)
    {
    }

    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        boost::asio::async_read(socket_, boost::asio::buffer(buffer_), boost::asio::transfer_at_least(1),
                                boost::bind(&connection::handle_read, shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }

private:
    void handle_read(const boost::system::error_code& e,
                                 std::size_t bytes_transferred) {
        if (!e) {
            if (bytes_transferred != 0) {
                boost::asio::async_write(socket_, boost::asio::buffer(buffer_),
                                         boost::bind(&connection::handle_write, shared_from_this(),
                                                     boost::asio::placeholders::error));
            } else {
                boost::asio::async_read(socket_, boost::asio::buffer(buffer_), boost::asio::transfer_at_least(1),
                                        boost::bind(&connection::handle_read, shared_from_this(),
                                                    boost::asio::placeholders::error,
                                                    boost::asio::placeholders::bytes_transferred));
            }
        }
    }

    void handle_write(const boost::system::error_code& e)
    {
        if (!e)
        {
            socket_.close();
        }
    }
};


class server : private boost::asio::noncopyable
{
public:
    server(const boost::asio::ip::tcp::endpoint &endpoint, size_t thread_pool_size)
            : thread_pool_size_(thread_pool_size),
              acceptor_(io_context_)

    {
        boost::system::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        acceptor_.bind(endpoint, ec);
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
        start_accept();
    }

    void run()
    {
        std::vector<boost::shared_ptr<boost::thread> > threads;
        for (std::size_t i = 0; i < thread_pool_size_; ++i)
        {
            boost::shared_ptr<boost::thread> thread(new boost::thread(
                    boost::bind(&boost::asio::io_context::run, &io_context_)));
            threads.push_back(thread);
        }
        for (auto & thread : threads)
            thread->join();
    }

private:
    void start_accept()
    {
        new_connection_.reset(new connection(io_context_));
        acceptor_.async_accept(new_connection_->socket(),
                               boost::bind(&server::handle_accept, this,
                                           boost::asio::placeholders::error));
    }

    void handle_accept(const boost::system::error_code& e)
    {
        if (!e)
        {
            new_connection_->start();
        }
        start_accept();
    }

    std::size_t thread_pool_size_;
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    connection::connection_ptr new_connection_;
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

