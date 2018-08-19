#ifndef UTILS_FUNCTION_H
#define UTILS_FUNCTION_H

namespace detail
{
    template<typename T>
    struct __FunctionImpl;

    template<typename R>
    struct __FunctionImpl<R()> {
        virtual R operator()() = 0;
    };

    template<typename R, typename A1>
    struct __FunctionImpl<R(A1)> {
        virtual R operator()(A1 a1) = 0;
    };
}

template<typename T>
class Function;

template<typename R>
class Function<R()>
{
public:
    typedef detail::__FunctionImpl<R()> impl_t;
    // TODO 引用计数
    Function() : impl_(nullptr) {}
    Function(impl_t* impl) : impl_(impl) {}

public:
    R operator()() {
        return (*impl_)();
    }

private:
    impl_t* impl_;
};

template<typename R, typename A1>
class Function<R(A1)>
{
public:
    // TODO 引用计数
    typedef detail::__FunctionImpl<R(A1)> impl_t;
    Function() : impl_(nullptr) {}
    Function(impl_t* impl) : impl_(impl) {}

public:
    R operator()(A1 a1) {
        return (*impl_)(a1);
    }

private:
    impl_t* impl_;
};

namespace detail
{
    template<typename T, typename R>
    class __MemberFunctionImpl0 : public __FunctionImpl<R()>
    {
    public:
        typedef R(T::*memfun_t)();
        __MemberFunctionImpl0(T* obj, memfun_t f)
            : obj_(obj), fun_(f) {}

        virtual R operator()() {
            return (obj_->*fun_)();
        }

    private:
        T* obj_;
        memfun_t fun_;
    };

    template<typename R>
    class __StaticFunctionImpl0 : public __FunctionImpl<R()>
    {
    public:
        typedef R(*fun_t)();
        __StaticFunctionImpl0(fun_t f)
            : fun_(f) {}

        virtual R operator()() {
            return (*fun_)();
        }

    private:
        fun_t fun_;
    };

    template<typename T, typename R, typename A1>
    class __MemberFunctionImpl1 : public __FunctionImpl<R(A1)>
    {
    public:
        typedef R(T::*memfun_t)(A1);
        __MemberFunctionImpl1(T* obj, memfun_t f)
            : obj_(obj), fun_(f) {}

        virtual R operator()(A1 a1) {
            return (obj_->*fun_)(a1);
        }
    private:
        T* obj_;
        memfun_t fun_;
    };

    template<typename R, typename A1>
    class __StaticFunctionImpl1 : public __FunctionImpl<R(A1)>
    {
    public:
        typedef R(*fun_t)(A1);
        __StaticFunctionImpl1(fun_t f)
            : fun_(f) {}

        virtual R operator()(A1 a1) {
            return (*fun_)(a1);
        }

    private:
        fun_t fun_;
    };
}

template<typename T, typename R>
Function<R()> Bind(R(T::*fun)(), T* obj)
{
    return Function<R()>(new detail::__MemberFunctionImpl0<T, R>(obj, fun));
}

template<typename R>
Function<R()> Bind(R(*fun)())
{
    return Function<R()>(new detail::__StaticFunctionImpl0<R>(fun));
}

template<typename T, typename R, typename A1>
Function<R(A1)> Bind(R(T::*fun)(A1), T* obj)
{
    return Function<R(A1)>(new detail::__MemberFunctionImpl1<T, R, A1>(obj, fun));
}

template<typename R, typename A1>
Function<R(A1)> Bind(R(*fun)(A1))
{
    return Function<R(A1)>(new detail::__StaticFunctionImpl1<R, A1>(fun));
}


#endif
