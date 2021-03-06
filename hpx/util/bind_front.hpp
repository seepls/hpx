//  Copyright (c) 2011-2012 Thomas Heller
//  Copyright (c) 2013-2019 Agustin Berge
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_UTIL_BIND_FRONT_HPP
#define HPX_UTIL_BIND_FRONT_HPP

#include <hpx/config.hpp>
#include <hpx/traits/get_function_address.hpp>
#include <hpx/traits/get_function_annotation.hpp>
#include <hpx/util/decay.hpp>
#include <hpx/util/detail/pack.hpp>
#include <hpx/util/invoke.hpp>
#include <hpx/util/one_shot.hpp>
#include <hpx/util/result_of.hpp>
#include <hpx/util/tuple.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx { namespace util
{
    namespace detail
    {
        template <typename F, typename Ts, typename ...Us>
        struct invoke_bound_front_result;

        template <typename F, typename ...Ts, typename ...Us>
        struct invoke_bound_front_result<F, util::tuple<Ts...>, Us...>
          : util::invoke_result<F, Ts..., Us...>
        {};

        ///////////////////////////////////////////////////////////////////////
        template <typename F, typename Ts, typename Is>
        struct bound_front_impl;

        template <typename F, typename ...Ts, std::size_t ...Is>
        struct bound_front_impl<F, util::tuple<Ts...>, pack_c<std::size_t, Is...>>
        {
            template <typename ...Us>
            HPX_CXX14_CONSTEXPR HPX_HOST_DEVICE
            typename invoke_bound_front_result<
                F&,
                util::tuple<Ts&...>,
                Us&&...
            >::type operator()(Us&&... vs) &
            {
                return HPX_INVOKE(_f,
                    util::get<Is>(_args)..., std::forward<Us>(vs)...);
            }

            template <typename ...Us>
            HPX_CONSTEXPR HPX_HOST_DEVICE
            typename invoke_bound_front_result<
                F const&,
                util::tuple<Ts const&...>,
                Us&&...
            >::type operator()(Us&&... vs) const&
            {
                return HPX_INVOKE(_f,
                    util::get<Is>(_args)..., std::forward<Us>(vs)...);
            }

            template <typename ...Us>
            HPX_CXX14_CONSTEXPR HPX_HOST_DEVICE
            typename invoke_bound_front_result<
                F&&,
                util::tuple<Ts&&...>,
                Us&&...
            >::type operator()(Us&&... vs) &&
            {
                return HPX_INVOKE(std::move(_f),
                    util::get<Is>(std::move(_args))..., std::forward<Us>(vs)...);
            }

            template <typename ...Us>
            HPX_CONSTEXPR HPX_HOST_DEVICE
            typename invoke_bound_front_result<
                F const&&,
                util::tuple<Ts const&&...>,
                Us&&...
            >::type operator()(Us&&... vs) const&&
            {
                return HPX_INVOKE(std::move(_f),
                    util::get<Is>(std::move(_args))..., std::forward<Us>(vs)...);
            }

            F _f;
            util::tuple<Ts...> _args;
        };

        template <typename F, typename ...Ts>
        class bound_front
          : private bound_front_impl<
                F, util::tuple<Ts...>,
                typename detail::make_index_pack<sizeof...(Ts)>::type
            >
        {
            using base_type = detail::bound_front_impl<
                F, util::tuple<Ts...>,
                typename detail::make_index_pack<sizeof...(Ts)>::type
            >;

        public:
            bound_front() {} // needed for serialization

            template <typename F_, typename ...Ts_, typename =
                typename std::enable_if<
                    std::is_constructible<F, F_>::value
                >::type>
            HPX_CONSTEXPR explicit bound_front(F_&& f, Ts_&&... vs)
              : base_type{
                    std::forward<F_>(f),
                    util::forward_as_tuple(std::forward<Ts_>(vs)...)}
            {}

#if !defined(__NVCC__) && !defined(__CUDACC__)
            bound_front(bound_front const&) = default;
            bound_front(bound_front&&) = default;
#else
            HPX_CONSTEXPR HPX_HOST_DEVICE bound_front(bound_front const& other)
              : base_type{other}
            {}

            HPX_CONSTEXPR HPX_HOST_DEVICE bound_front(bound_front&& other)
              : base_type{std::move(other)}
            {}
#endif

            bound_front& operator=(bound_front const&) = delete;

            using base_type::operator();

            template <typename Archive>
            void serialize(Archive& ar, unsigned int const /*version*/)
            {
                ar & _f;
                ar & _args;
            }

            std::size_t get_function_address() const
            {
                return traits::get_function_address<F>::call(_f);
            }

            char const* get_function_annotation() const
            {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
                return traits::get_function_annotation<F>::call(_f);
#else
                return nullptr;
#endif
            }

#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
            util::itt::string_handle get_function_annotation_itt() const
            {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
                return traits::get_function_annotation_itt<F>::call(_f);
#else
                static util::itt::string_handle sh("bound_front");
                return sh;
#endif
            }
#endif

        private:
            using base_type::_f;
            using base_type::_args;
        };
    }

    template <typename F, typename ...Ts>
    HPX_CONSTEXPR detail::bound_front<
        typename std::decay<F>::type,
        typename std::decay<Ts>::type...>
    bind_front(F&& f, Ts&&... vs) {
        typedef detail::bound_front<
            typename std::decay<F>::type,
            typename std::decay<Ts>::type...
        > result_type;

        return result_type(std::forward<F>(f), std::forward<Ts>(vs)...);
    }

    // nullary functions do not need to be bound again
    template <typename F>
    HPX_CONSTEXPR typename std::decay<F>::type
    bind_front(F&& f) {
        return std::forward<F>(f);
    }
}}

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    template <typename F, typename ...Ts>
    struct get_function_address<util::detail::bound_front<F, Ts...> >
    {
        static std::size_t
            call(util::detail::bound_front<F, Ts...> const& f) noexcept
        {
            return f.get_function_address();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename F, typename ...Ts>
    struct get_function_annotation<util::detail::bound_front<F, Ts...> >
    {
        static char const*
            call(util::detail::bound_front<F, Ts...> const& f) noexcept
        {
            return f.get_function_annotation();
        }
    };

#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
    template <typename F, typename ...Ts>
    struct get_function_annotation_itt<util::detail::bound_front<F, Ts...> >
    {
        static util::itt::string_handle
            call(util::detail::bound_front<F, Ts...> const& f) noexcept
        {
            return f.get_function_annotation_itt();
        }
    };
#endif
#endif
}}

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace serialization
{
    // serialization of the bound_front object
    template <typename Archive, typename F, typename ...Ts>
    void serialize(
        Archive& ar
      , ::hpx::util::detail::bound_front<F, Ts...>& bound
      , unsigned int const version = 0)
    {
        bound.serialize(ar, version);
    }
}}

#endif
