#pragma once
// Keep these settings consistent in every translation unit using the SDK.
#if defined(ASIO_SEPARATE_COMPILATION) || defined(ASIO_DYN_LINK)
#error ptgl Remote requires header-only standalone Asio
#endif
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#ifndef _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_STL_
#endif
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
namespace ptgl::remote::detail {
using WsServer = websocketpp::server<websocketpp::config::asio>;
using WsClient = websocketpp::client<websocketpp::config::asio_client>;
using Handle = websocketpp::connection_hdl;
using ErrorCode = websocketpp::lib::error_code;
template<class Endpoint> inline void configureSocket(Endpoint& endpoint) {
    endpoint.clear_access_channels(websocketpp::log::alevel::all);
    endpoint.clear_error_channels(websocketpp::log::elevel::all);
    endpoint.set_max_message_size(Limits::maxPayload+48);
    endpoint.set_open_handshake_timeout(5000);
    endpoint.set_close_handshake_timeout(1000);
    endpoint.init_asio();
}
}
