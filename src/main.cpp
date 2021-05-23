#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/json/src.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

int ret_code = EXIT_SUCCESS;

// Report a failure
void fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Pretty Print
void Pretty_print(std::ostream& os, boost::json::value const& jv, std::string* indent = nullptr)
{
    std::string indent_;
    if (!indent)
        indent = &indent_;
    switch (jv.kind())
    {
    case boost::json::kind::object:
    {
        os << "{\n";
        indent->append(4, ' ');
        auto const& obj = jv.get_object();
        if (!obj.empty())
        {
            auto it = obj.begin();
            for (;;)
            {
                os << *indent << boost::json::serialize(it->key()) << " : ";
                Pretty_print(os, it->value(), indent);
                if (++it == obj.end())
                    break;
                os << ",\n";
            }
        }
        os << "\n";
        indent->resize(indent->size() - 4);
        os << *indent << "}";
        break;
    }

    case boost::json::kind::array:
    {
        os << "[\n";
        indent->append(4, ' ');
        auto const& arr = jv.get_array();
        if (!arr.empty())
        {
            auto it = arr.begin();
            for (;;)
            {
                os << *indent;
                Pretty_print(os, *it, indent);
                if (++it == arr.end())
                    break;
                os << ",\n";
            }
        }
        os << "\n";
        indent->resize(indent->size() - 4);
        os << *indent << "]";
        break;
    }

    case boost::json::kind::string:
    {
        os << boost::json::serialize(jv.get_string());
        break;
    }

    case boost::json::kind::uint64:
        os << jv.get_uint64();
        break;

    case boost::json::kind::int64:
        os << jv.get_int64();
        break;

    case boost::json::kind::double_:
        os << jv.get_double();
        break;

    case boost::json::kind::bool_:
        if (jv.get_bool())
            os << "true";
        else
            os << "false";
        break;

    case boost::json::kind::null:
        os << "null";
        break;
    }

    if (indent->empty())
        os << "\n";
}

// Echoes back all received WebSocket messages
void Do_session(websocket::stream<beast::tcp_stream>& ws, net::yield_context yield)
{
    beast::error_code ec;

    // Set suggested timeout settings for the websocket
    ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    ws.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& res)
        {
            res.set(http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) +
                " websocket-server-coro");
        }));

    // Accept the websocket handshake
    ws.async_accept(yield[ec]);

    if (ec)
        return fail(ec, "accept");

    for (;;)
    {
        // This buffer will hold the incoming message
        beast::flat_buffer buffer;

        // Read a message
        ws.async_read(buffer, yield[ec]);

        // This indicates that the session was closed
        if (ec == websocket::error::closed)
            break;

        if (ec)
            return fail(ec, "read");

        std::cout << "Thread ID: " << std::this_thread::get_id() << std::endl;

        // Parse the message
        try
        {
            auto raw_json_frame = boost::json::parse(boost::beast::buffers_to_string(buffer.data()));
            Pretty_print(std::cout, raw_json_frame);
        }
        catch (std::exception& ex)
        {
            std::cerr << "Exception parsing json frame: " << ex.what() << std::endl; // swallow for now
        }

        // Echo the message back
        ws.text(ws.got_text());
        ws.async_write(buffer.data(), yield[ec]);
        if (ec)
            return fail(ec, "write");
    }
}

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
void Do_listen(net::io_context& ioc, tcp::endpoint endpoint, net::yield_context yield)
{
    beast::error_code ec;

    // Open the acceptor
    tcp::acceptor acceptor(ioc);
    acceptor.open(endpoint.protocol(), ec);
    if (ec)
        return fail(ec, "open");

    // Allow address reuse
    acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec)
        return fail(ec, "set_option");

    // Bind to the server address
    acceptor.bind(endpoint, ec);
    if (ec)
        return fail(ec, "bind");

    // Start listening for connections
    acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec)
        return fail(ec, "listen");

    for (;;)
    {
        tcp::socket socket(ioc);
        acceptor.async_accept(socket, yield[ec]);
        if (ec)
            fail(ec, "accept");
        else
            boost::asio::spawn(
                acceptor.get_executor(),
                std::bind(
                    &Do_session,
                    websocket::stream<
                    beast::tcp_stream>(std::move(socket)),
                    std::placeholders::_1));
    }
}

void Socketcom_server_app(void)
{
    auto const port = 81U;
    auto const threads = 1U;

    // The io_context is required for all I/O
    net::io_context ioc(threads);

    // Spawn a listening port
    boost::asio::spawn(ioc, std::bind(&Do_listen,
        std::ref(ioc),
        tcp::endpoint{ boost::asio::ip::tcp::v4(), port },
        std::placeholders::_1));

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
    {
        v.emplace_back(
            [&ioc]
            {
                ioc.run();
            });
    }

    ioc.run();
}

int main(int argc, char* argv[])
{
    /* Command line arguments currently unused*/
    (void)argc;
    (void)argv;

    try
    {
        Socketcom_server_app();
    }
    catch (std::exception& ex)
    {
        std::cerr << "Unhandled Exception: " << ex.what() << std::endl;
        ret_code = EXIT_FAILURE;
    }

    return ret_code;
}
