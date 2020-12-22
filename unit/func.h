//
// Created by explorer on 9/11/19.
//

#ifndef COROUTINE_FUNC_H
#define COROUTINE_FUNC_H
#include <tuple>
#include <functional>

class Func {
public:
    virtual void operator()() = 0;

    virtual ~Func() = default;
};

template<class R, class ... ARGS>
class func : public Func {
public:
    R r;
    std::tuple<ARGS...> arg_list;

    std::function<R(ARGS...)> call_func;

    static func make_func(R (*f)(ARGS ...), ARGS ... arg) {
        return func(f, arg...);
    }

    static func make_func(std::function<R(ARGS...)> f, ARGS ... arg) {
        return func(f, arg...);
    }

    static func *new_func(R (*f)(ARGS ...), ARGS ... arg) {
        return new func(f, arg...);
    }

    static func *new_func(std::function<R(ARGS...)> f, ARGS ... arg) {
        return new func(f, arg...);
    }

    template<std::size_t... I>
    void call(std::index_sequence<I...>) {
        r = call_func(std::get<I>(arg_list)...);
    }

    void operator()() override {
        call(std::make_index_sequence<sizeof...(ARGS)>());
    }

    ~func() {}

    explicit func(std::function<R(ARGS...)> f, ARGS ... arg) {
        call_func = f;
        arg_list = std::make_tuple(arg...);
    }

    explicit func(R (*f)(ARGS...), ARGS ... arg) {
        call_func = f;
        arg_list = std::make_tuple(arg...);
    }

};

// specialization for func which return void
template<class  ... ARGS>
class func<void, ARGS...> : public Func {
public:
    int r = 0;
    std::tuple<ARGS...> arg_list;

    std::function<void(ARGS...)> call_func;

    static func make_func(void (*f)(ARGS ...), ARGS ... arg) {
        return func(f, arg...);
    }

    static func make_func(std::function<void(ARGS...)> f, ARGS ... arg) {
        return func(f, arg...);
    }

    static func *new_func(void (*f)(ARGS ...), ARGS ... arg) {
        return new func(f, arg...);
    }

    static func *new_func(std::function<void(ARGS...)> f, ARGS ... arg) {
        return new func(f, arg...);
    }

    template<std::size_t... I>
    void call(std::index_sequence<I...>) {
        call_func(std::get<I>(arg_list)...);
    }

    void operator()() override {
        call(std::make_index_sequence<sizeof...(ARGS)>());
    }

    ~func() {}

    explicit func(std::function<void(ARGS...)> f, ARGS ... arg) {
        call_func = f;
        arg_list = std::make_tuple(arg...);
    }

    explicit func(void (*f)(ARGS...), ARGS ... arg) {
        call_func = f;
        arg_list = std::make_tuple(arg...);
    }

};

template<class R, class ... ARGS>
func<R, ARGS...> make_func(R (*f)(ARGS ...), ARGS ... arg) {
    return func<R, ARGS...>::make_func(f, arg...);
}

template<class R, class ... ARGS>
func<R, ARGS...> make_func(std::function<R(ARGS...)> f, ARGS ... arg) {
    return func<R, ARGS...>::make_func(f, arg...);
}

template<class R, class ... ARGS>
func<R, ARGS...> *new_func(R (*f)(ARGS ...), ARGS ... arg) {
    return func<R, ARGS...>::new_func(f, arg...);
}

template<class R, class ... ARGS>
func<R, ARGS...> *new_func(std::function<R(ARGS...)> f, ARGS ... arg) {
    return func<R, ARGS...>::new_func(f, arg...);
}

#endif //COROUTINE_FUNC_H
