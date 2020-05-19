# **Stream-like and monadic api for asio (Toys)**

## 1. Finished 

* abstractly approaching tcp socket as a monadic message stream ( M string )
* range (pipe-like) operation combinator ( transform , filter etc )

## 2. TODO

* wait for c++20 ( concept , coruntine )
* more general approach for asio , not only support read/write-buffer for TCP server socket
* supply more stream-like & functional combinator ( for-each , flat_map , zip ... etc )

## 3. example 

most simplest socket message stream

```C++
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

```

make a value stream for pipine testing 

```C++
    using sess::filter , sess::transform , sess::Session ;
    auto pipeline = 
        sess::make_value_stream({114,514,114514}) 
        | filter    ([](int i)      { if(i == 114514) { fmt::print("fuck {}\n" , i); return true; }} ) 
        | transform ([](int && i)   { fmt::print("jojojojojojo {}\n" , i) ;return 24242424; } );
    shared_ptr<Session> sess{};
    pipeline.run_msg_processor(sess);

```