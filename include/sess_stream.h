#pragma once
#include <asio.hpp>
#include <type_traits>
#include <functional>
#include <vector>

#include "session_server.h"

namespace sess{

using std::move;
using std::invoke;
using std::forward;

struct none_t{};

// ============================================================================

template<class val_t>
class SessionStream {

    using handler_func_t    = std::function<void(val_t && )>;
    using processor_func_t  = std::function<void(std::shared_ptr<Session> & ,handler_func_t & )>;

private:

    processor_func_t    processor_ {};
    handler_func_t      handler_ {};

public:

    using  value_type = val_t;

    template<class Processor>
    SessionStream(Processor && p) noexcept
    :processor_(std::forward<Processor>(p)){}

    void run_msg_processor(std::shared_ptr<Session> & sess){
        if(processor_)processor_(sess , handler_);
    }

    void set_handler(handler_func_t f) noexcept{
        handler_ = move(f);
    }
};

template<class Processor>
SessionStream(Processor &&) -> SessionStream<typename Processor::result_type>;

template<class T>
void registe_sevice(Service & s , SessionStream<T> & pipeline ) {
    s.set_accepted([&](std::shared_ptr<Session> && sess){
        pipeline.run_msg_processor(sess);
    });
}

// ================================ processor ====================================

template<class T ,class val_t>
struct processor_impl{

    using result_type       = std::conditional_t<std::is_void_v<val_t> , none_t , val_t>;
    using handler_func_t    = std::function<void(result_type &&)>;
    
    void operator() (std::shared_ptr<Session> & sess , handler_func_t & t){
        static_cast<T *>(this)->process(sess , t);
    }
};

/* ======== session source ===========*/
struct sess_processor : processor_impl< sess_processor , std::string >{
    void process(std::shared_ptr<Session> & sess , handler_func_t & handler){
        if(handler) sess->start(move(handler));
    }
};

/* ======== value source (for test)============= */
template<class Value>
struct value_processor : processor_impl< value_processor<Value> , Value >{

    value_processor(Value & v) noexcept : v_(v) {}
    value_processor(Value &&v) noexcept : v_(move(v)){}

    using base_t = processor_impl< value_processor<Value> , Value >;

    void process(std::shared_ptr<Session> & sess , typename base_t::handler_func_t & handler) {
        //unused(sess)
        if(handler) handler (Value(v_));
    }
    
    Value v_;
};

/* =========== value stream source (for test) ================ */
template<class L ,class V>
struct list_processor : processor_impl< list_processor<L , V> , V> {

    list_processor(L && l) noexcept : list_(move(l)){} 
    template<class Value>
    list_processor(std::initializer_list<Value> l) noexcept : list_(l){}

    using base_t = processor_impl<list_processor<L,V> , V>;
    void process(std::shared_ptr<Session> & sess , typename base_t::handler_func_t & handler){
        //unused(sess)
        if(handler) for(auto && v : list_) { handler (move(v)) ;}
    }
    L list_;
};

template<class  Value>
list_processor(std::initializer_list<Value> )-> list_processor<std::vector<Value>  ,  Value>; 

/* ========== transform combinator ================ */

template<class S , class F>
using trans_result_t = std::invoke_result_t<F , typename S::value_type >;

template<class Sender , class Transformer >
struct transform_processor 
:processor_impl < 
    transform_processor<Sender , Transformer>,
    trans_result_t<Sender, Transformer>
    >{

    using msg_t     = typename Sender::value_type;
    using base_t    = processor_impl < transform_processor<Sender , Transformer>,trans_result_t<Sender, Transformer>>;

    transform_processor(Sender && s , Transformer f) noexcept 
    : sender_(move(s)) , transformer_(move(f)){}

    void process(std::shared_ptr<Session> & sess , typename base_t::handler_func_t & handler){
        sender_.set_handler([this , &handler](msg_t &&  msg){
            if constexpr ( std::is_same_v< typename base_t::result_type , none_t> ){
                transformer_(move(msg));
                if(handler) handler(none_t{});
            }
            else {
                auto && val = transformer_(move(msg));
                if(handler) handler(move(val));
            }
        });
        sender_.run_msg_processor(sess);
    }

    Sender      sender_;
    Transformer transformer_;
};

template<class Transformer>
struct transform{
    transform(Transformer && f) noexcept : t_(move(f)){}
    Transformer t_;
};

template <class V , class T>
auto operator | (SessionStream<V> && s , transform<T> && t){
    return SessionStream(transform_processor(move(s) , move(t.t_)));
}

/* ============== filter combinator ================== */

template<class Sender ,class Predicate >
struct filter_processor : processor_impl < filter_processor<Sender , Predicate> , typename Sender::value_type> {

    using msg_t = typename Sender::value_type;
    using base_t= processor_impl < filter_processor<Sender , Predicate> , typename Sender::value_type> ;

    filter_processor(Sender && s , Predicate p) noexcept 
    :sender_(move(s)) , predicate_(move(p)) {}

    void process(std::shared_ptr<Session> & sess , typename base_t::handler_func_t & handler){
        sender_.set_handler([this , &handler] (msg_t && msg){
            if(handler && std::invoke(predicate_ , msg)) handler(move(msg));
        });
        sender_.run_msg_processor(sess);
    }

    Sender      sender_;
    Predicate   predicate_;
};

template<class P>
struct filter {
    filter(P && p):predicate_(move(p)){}
    P predicate_;
};
template<class V , class P>
auto operator | (SessionStream<V> && s , filter<P> && f){
    return SessionStream(filter_processor(move(s) , move(f.predicate_))); 
}

/* ================ then =============================== */
template<class Sender , class Action >
struct then_processor : processor_impl<then_processor <Sender , Action> , typename Sender::value_type >{
    using msg_t  = typename Sender::value_type;
    using base_t = processor_impl<filter_processor <Sender , Action> , typename Sender::value_type >;

    then_processor(Sender && s , Action act) noexcept
    :sender_(move(s)) , action_(act) {}

    void process(std::shared_ptr<Session> & sess , typename base_t::handler_func_t & handler){
        sender_.set_handler([this , &handler ](msg_t && msg){
            action_ (msg) ;
            if(handler) handler(move(msg));
        });
        sender_.run_msg_processor(sess);
    }

    Sender sender_;
    Action action_;
};

template<class Action>
struct then{
    then (Action && act):act_(move(act)){}
    Action act_;
};

template <class V , class A>
auto operator | (SessionStream<V> && s , then<A> && act){
    return SessionStream(then_processor(move(s) , move(act.act_)));
}


/* ================ writer =============================== */

struct write_processor : processor_impl<write_processor , none_t>{

    write_processor(SessionStream<std::string> && s ) noexcept 
    :sender_(move(s)){}

    void process(std::shared_ptr<Session> & sess , handler_func_t & handler){
        sender_.set_handler([this ,sess , &handler](std::string && s){
            sess->write(move(s) , [&](){
                if(handler) handler (none_t{});
            });
        });
        sender_.run_msg_processor(sess);
    }

    SessionStream<std::string> sender_;
};

struct write{};

auto operator | (SessionStream<std::string> && s , write w ){
    return SessionStream(write_processor(move(s)));
}

//===============================================================================

auto make_message_stream() {
    return SessionStream(sess_processor{});
}
template<class Value>
auto make_value(Value && v){
    return SessionStream(value_processor{std::forward<Value>(v)});
}

template<class L>
auto make_value_stream(L && l){
    return SessionStream (list_processor(std::forward<L>(l)));
}

template<class V>
auto make_value_stream(std::initializer_list<V> l){
    return SessionStream (list_processor(l));
}

}
