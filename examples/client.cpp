#include <iostream>
#include <asio.hpp>
#include <string_view>
#include <array>
#include <type_traits>
#include <fmt/format.h>

using namespace std;
using asio::ip::tcp;

using endpoints_t = tcp::resolver::results_type;
auto error_format = "[ERROR in {}]:{}\n";

class Client{
public :
    Client(asio::io_context & io_context) noexcept
    :socket_(io_context)  {}

    void connect(endpoints_t endpoints) {
        asio::async_connect(
            socket_ ,endpoints , 
            [this](error_code err , tcp::endpoint endpoint){
                if(!err) write();
                else fmt::print(error_format  ,"connect",system_error(err).what()); 
            }
        );
    }

    void read(){
        socket_.async_read_some(asio::buffer(buffer_) , [this](error_code err , size_t len){
            if(!err) {
                fmt::print("MESSAGE >> {}\n" , string_view(buffer_.data() , len));
                write();
            }
            else 
                fmt::print(error_format , "read" ,system_error(err).what());
        });
    }

    void write(){
        size_t len = 0;
        do{fmt::print("MESSAGE << ");}
        while(cin.getline(buffer_.data() ,buffer_.size()) && (len = strlen(buffer_.data())) == 0);
        if(!cin) return ;

        socket_.async_send(
            asio::buffer(buffer_.data() , len),[this](error_code err , size_t len){
                if(!err) read();
                else fmt::print(error_format , "write",system_error(err).what());
            }
        );
    }

private:
    tcp::socket socket_;
    array<char , 1024> buffer_{};

};

int main() {
    asio::io_context io_context{};
    tcp::resolver resolver{io_context};
    Client client{io_context};

    try{
        client.connect(resolver.resolve(tcp::v4() , "","11451"));
        io_context.run();
    }
    catch(exception & e){
        fmt::print("[ERROR]:{}" , e.what());
    }
}
