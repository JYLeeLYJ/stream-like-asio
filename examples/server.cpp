#include <iostream>

#include "sess_stream.h"
#include "session_server.h"

using namespace std;

constexpr uint16_t  PORT  = 11451; 

void test(){
    using sess::filter , sess::transform , sess::Session ;
    auto pipeline = 
        sess::make_value_stream({114,514,114514}) 
        | filter    ([](int i)      { if(i == 114514) fmt::print("fuck {}\n" , i); return true;}) 
        | transform ([](int && i)   { fmt::print("jojojojojojo {}\n" , i) ;return 24242424; } );
    shared_ptr<Session> sess{};
    pipeline.run_msg_processor(sess);
}

int main(){

    // test();
    using sess::filter , sess::transform ,sess::write , sess::then ;
    
    //echo service
    auto pipeline = 
        sess::make_message_stream() 
        | then([](auto && s) {fmt::print("read message : {}\n" , s) ;})
        | write();
    
    try{
        asio::io_context io_context{};
        sess::Service server(io_context , PORT );
        sess::registe_sevice(server , pipeline);
        io_context.run();    
    }
    catch(exception & e){
        fmt::print("[ERROR]:{}\n" , e.what());
    }
}