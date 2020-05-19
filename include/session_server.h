#pragma once
#include <memory>
#include <array>

#include <fmt/format.h>
#include <asio.hpp>

namespace sess{

constexpr size_t    BUF_SIZE    = 1024;

inline auto constexpr error_format = "[ERROR]:{}\n";

using asio::ip::tcp;
using std::move;

class Session : public std::enable_shared_from_this<Session>{
public:  

    Session(tcp::socket socket) noexcept
    :socket_(move(socket)){}

    Session(Session && ) = default;
    Session(const Session &) = delete ;

    void start(std::function<void(std::string&&)> f){
        emit_func_ = move(f);
        read(); 
    }

    void read(){
        socket_.async_read_some(
            asio::buffer(rbuffer_),
            [this ,self = shared_from_this()](std::error_code err , std::size_t len){
                if(!err)emit_func_(std::string(rbuffer_.data() , len));
                else fmt::print(error_format , std::system_error(err).what());
                read();
            }
        );
    }

    template<class FuncAndThen >
    void write(std::string data , FuncAndThen && then){
        auto len = min(data.length() * sizeof(decltype(data)::value_type), BUF_SIZE);
        memcpy(wbuffer_.data() , data.data() , len);
        asio::async_write(
            socket_ ,
            asio::buffer(wbuffer_ , len ) , 
            [this,self = shared_from_this(), and_then = move(then)](error_code err , size_t len){
                if(!err) and_then () ;
                else fmt::print(error_format , std::system_error(err).what());
            }
        );
    }
    
    Session & session() {return *this;}

private:
    tcp::socket                         socket_;
    std::array<char , BUF_SIZE>         rbuffer_{};
    std::array<char , BUF_SIZE>         wbuffer_{};
    std::function<void(std::string &&)> emit_func_{};
};

class Service{
public:
    Service(asio::io_context & context , uint16_t port) noexcept
        :acceptor_(context , tcp::endpoint{tcp::v4() , port}){        
    }

    Service(const Service &) = delete;
    Service(Service && ) = default;

    void accept(){
        acceptor_.async_accept([this](std::error_code err , tcp::socket socket){
            if(!err) emit_accepted_(std::make_shared<Session>(move(socket)));
            this->accept(); 
        });
    }

    void set_accepted(std::function<void(std::shared_ptr<Session> && )> f){
        emit_accepted_ = move(f);
        accept();
    }

private:
    tcp::acceptor   acceptor_;
    std::function<void(std::shared_ptr<Session> &&)>  emit_accepted_{};
};

}

