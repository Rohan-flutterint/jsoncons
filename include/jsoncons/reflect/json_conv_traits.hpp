// Copyright 2013-2025 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_REFLECT_JSON_CONV_TRAITS_HPP
#define JSONCONS_REFLECT_JSON_CONV_TRAITS_HPP

#include <algorithm> // std::swap
#include <array>
#include <bitset> // std::bitset
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator> // std::iterator_traits, std::input_iterator_tag
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits> // std::enable_if
#include <utility>
#include <valarray>
#include <vector>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/conv_error.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/utility/bigint.hpp>
#include <jsoncons/utility/byte_string.hpp>
#include <jsoncons/utility/more_type_traits.hpp>
#include <jsoncons/value_converter.hpp>
#include <jsoncons/conversion_result.hpp>
#include <jsoncons/json_type_traits.hpp>

#if defined(JSONCONS_HAS_STD_VARIANT)
  #include <variant>
#endif

namespace jsoncons {
namespace reflect {

    template <typename T>
    struct is_json_conv_traits_declared : public is_json_type_traits_declared<T>
    {};

    // json_conv_traits

    template <typename T>
    struct unimplemented : std::false_type
    {};

    template <typename Json,typename T,typename Enable=void>
    struct json_conv_traits
    {
        using result_type = conversion_result<T>;
        using allocator_type = typename Json::allocator_type;

        static constexpr bool is_compatible = false;

        static constexpr bool is(const Json& j) noexcept
        {
            return json_type_traits<Json,T>::is(j);
        }

        static result_type try_as(const Json& j)
        {
            
            JSONCONS_TRY
            {
                return result_type(json_type_traits<Json,T>::as(j));
            }
            JSONCONS_CATCH (const ser_error& ec)
            {
                return result_type(jsoncons::unexpect, ec.code());
            }
        }

        static Json to_json(const T& val, const allocator_type& alloc = allocator_type())
        {
            return json_type_traits<Json,T>::to_json(val, alloc);
        }
    };

namespace detail {

template <typename Json,typename T>
using
traits_can_convert_t = decltype(json_conv_traits<Json,T>::can_convert(Json()));

template <typename Json,typename T>
using
has_can_convert = ext_traits::is_detected<traits_can_convert_t, Json, T>;

    template <typename T>
    struct invoke_can_convert
    {
        template <typename Json>
        static 
        typename std::enable_if<has_can_convert<Json,T>::value,bool>::type
        can_convert(const Json& j) noexcept
        {
            return json_conv_traits<Json,T>::can_convert(j);
        }
        template <typename Json>
        static 
        typename std::enable_if<!has_can_convert<Json,T>::value,bool>::type
        can_convert(const Json& j) noexcept
        {
            return json_conv_traits<Json,T>::is(j);
        }
    };

    // is_json_conv_traits_unspecialized
    template <typename Json,typename T,typename Enable = void>
    struct is_json_conv_traits_unspecialized : std::false_type {};

    // is_json_conv_traits_unspecialized
    template <typename Json,typename T>
    struct is_json_conv_traits_unspecialized<Json,T,
        typename std::enable_if<!std::integral_constant<bool, json_conv_traits<Json, T>::is_compatible>::value>::type
    > : std::true_type {};

    // is_compatible_array_type
    template <typename Json,typename T,typename Enable=void>
    struct is_compatible_array_type : std::false_type {};

    template <typename Json,typename T>
    struct is_compatible_array_type<Json,T, 
        typename std::enable_if<!std::is_same<T,typename Json::array>::value &&
        ext_traits::is_array_like<T>::value && 
        !is_json_conv_traits_unspecialized<Json,typename std::iterator_traits<typename T::iterator>::value_type>::value
    >::type> : std::true_type {};

} // namespace detail

    // is_json_conv_traits_specialized
    template <typename Json,typename T,typename Enable=void>
    struct is_json_conv_traits_specialized : is_json_type_traits_specialized<Json,T> {};

    template <typename Json,typename T>
    struct is_json_conv_traits_specialized<Json,T, 
        typename std::enable_if<!jsoncons::reflect::detail::is_json_conv_traits_unspecialized<Json,T>::value
    >::type> : std::true_type {};

    template <typename Json>
    struct json_conv_traits<Json, const typename std::decay<typename Json::char_type>::type*>
    {
        using char_type = typename Json::char_type;
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<const char_type*>;

        static bool is(const Json& j) noexcept
        {
            return j.is_string();
        }
        static result_type try_as(const Json& j)
        {
            return result_type{j.as_cstring()};
        }
        template <typename ... Args>
        static Json to_json(const char_type* s, Args&&... args)
        {
            return Json(s, semantic_tag::none, std::forward<Args>(args)...);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json,typename std::decay<typename Json::char_type>::type*>
    {
        using char_type = typename Json::char_type;
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_string();
        }
        template <typename ... Args>
        static Json to_json(const char_type* s, Args&&... args)
        {
            return Json(s, semantic_tag::none, std::forward<Args>(args)...);
        }
    };

    // integer

    template <typename Json,typename T>
    struct json_conv_traits<Json, T,
        typename std::enable_if<(ext_traits::is_signed_integer<T>::value && sizeof(T) <= sizeof(int64_t)) || (ext_traits::is_unsigned_integer<T>::value && sizeof(T) <= sizeof(uint64_t)) 
    >::type>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.template is_integer<T>();
        }
        static result_type try_as(const Json& j)
        {
            return j.template try_as_integer<T>();
        }

        static Json to_json(T val)
        {
            return Json(val, semantic_tag::none);
        }

        static Json to_json(T val, const allocator_type&)
        {
            return Json(val, semantic_tag::none);
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T,
        typename std::enable_if<(ext_traits::is_signed_integer<T>::value && sizeof(T) > sizeof(int64_t)) || (ext_traits::is_unsigned_integer<T>::value && sizeof(T) > sizeof(uint64_t)) 
    >::type>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.template is_integer<T>();
        }
        static result_type try_as(const Json& j)
        {
            return result_type(j.template as_integer<T>());
        }

        static Json to_json(T val, const allocator_type& alloc = allocator_type())
        {
            return Json(val, semantic_tag::none, alloc);
        }
    };

    template <typename Json, typename T>
    struct json_conv_traits<Json, T,
                            typename std::enable_if<std::is_floating_point<T>::value
    >::type>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.is_double();
        }
        static result_type try_as(const Json& j)
        {
            auto result = j.try_as_double();
            if (JSONCONS_UNLIKELY(!result))
            {
                return result_type(result.error().code());
            }
            return result_type(static_cast<T>(*result));
        }
        static Json to_json(T val)
        {
            return Json(val, semantic_tag::none);
        }
        static Json to_json(T val, const allocator_type&)
        {
            return Json(val, semantic_tag::none);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json,typename Json::object>
    {
        using json_object = typename Json::object;
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_object();
        }
        static Json to_json(const json_object& o)
        {
            return Json(o,semantic_tag::none);
        }
        static Json to_json(const json_object& o, const allocator_type&)
        {
            return Json(o,semantic_tag::none);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json,typename Json::array>
    {
        using json_array = typename Json::array;
        using allocator_type = typename Json::allocator_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_array();
        }
        static Json to_json(const json_array& a)
        {
            return Json(a, semantic_tag::none);
        }
        static Json to_json(const json_array& a, const allocator_type&)
        {
            return Json(a, semantic_tag::none);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json, Json>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<Json>;

        static bool is(const Json&) noexcept
        {
            return true;
        }
        static result_type try_as(const Json& j)
        {
            return result_type{j};
        }
        static Json to_json(const Json& val)
        {
            return val;
        }
        static Json to_json(const Json& val, const allocator_type&)
        {
            return val;
        }
    };

    template <typename Json>
    struct json_conv_traits<Json, jsoncons::null_type>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<typename jsoncons::null_type>;

        static bool is(const Json& j) noexcept
        {
            return j.is_null();
        }
        static result_type try_as(const Json& j)
        {
            if (!j.is_null())
            {
                return result_type{jsoncons::unexpect, conv_errc::not_jsoncons_null_type};
            }
            return result_type{jsoncons::null_type()};
        }
        static Json to_json(jsoncons::null_type)
        {
            return Json(jsoncons::null_type{}, semantic_tag::none);
        }
        static Json to_json(jsoncons::null_type, const allocator_type&)
        {
            return Json(jsoncons::null_type{}, semantic_tag::none);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json, bool>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<bool>;

        static bool is(const Json& j) noexcept
        {
            return j.is_bool();
        }
        static result_type try_as(const Json& j)
        {
            return result_type{j.as_bool()};
        }
        static Json to_json(bool val)
        {
            return Json(val, semantic_tag::none);
        }
        static Json to_json(bool val, const allocator_type&)
        {
            return Json(val, semantic_tag::none);
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T,typename std::enable_if<std::is_same<T, 
        std::conditional<!std::is_same<bool,std::vector<bool>::const_reference>::value,
                         std::vector<bool>::const_reference,
                         void>::type>::value>::type>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<bool>;

        static bool is(const Json& j) noexcept
        {
            return j.is_bool();
        }
        static result_type try_as(const Json& j)
        {
            return result_type{j.as_bool()};
        }
        static Json to_json(bool val)
        {
            return Json(val, semantic_tag::none);
        }
        static Json to_json(bool val, const allocator_type&)
        {
            return Json(val, semantic_tag::none);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json, std::vector<bool>::reference>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<bool>;

        static bool is(const Json& j) noexcept
        {
            return j.is_bool();
        }
        static result_type try_as(const Json& j)
        {
            return result_type{j.as_bool()};
        }
        static Json to_json(bool val)
        {
            return Json(val, semantic_tag::none);
        }
        static Json to_json(bool val, const allocator_type&)
        {
            return Json(val, semantic_tag::none);
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    ext_traits::is_string<T>::value &&
                                                    std::is_same<typename Json::char_type,typename T::value_type>::value>::type>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.is_string();
        }

        static result_type try_as(const Json& j)
        {
            return result_type{jsoncons::in_place, j.as_string()};
        }

        static Json to_json(const T& val)
        {
            return Json(val, semantic_tag::none);
        }

        static Json to_json(const T& val, const allocator_type& alloc)
        {
            return Json(val, semantic_tag::none, alloc);
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    ext_traits::is_string<T>::value &&
                                                    !std::is_same<typename Json::char_type,typename T::value_type>::value>::type>
    {
        using char_type = typename Json::char_type;
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.is_string();
        }

        static result_type try_as(const Json& j)
        {
            auto s = j.as_string();
            T val;
            unicode_traits::convert(s.data(), s.size(), val);
            return result_type(std::move(val));
        }

        static Json to_json(const T& val)
        {
            std::basic_string<char_type> s;
            unicode_traits::convert(val.data(), val.size(), s);

            return Json(s, semantic_tag::none);
        }

        static Json to_json(const T& val, const allocator_type& alloc)
        {
            std::basic_string<char_type> s;
            unicode_traits::convert(val.data(), val.size(), s);
            return Json(s, semantic_tag::none, alloc);
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    ext_traits::is_string_view<T>::value &&
                                                    std::is_same<typename Json::char_type,typename T::value_type>::value>::type>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.is_string_view();
        }

        static result_type try_as(const Json& j)
        {
            auto result = j.try_as_string_view();
            return result ? result_type(in_place, result.value().data(), result.value().size()) : result_type(unexpect, conv_errc::not_string); 
        }

        static Json to_json(const T& val)
        {
            return Json(val, semantic_tag::none);
        }

        static Json to_json(const T& val, const allocator_type& alloc)
        {
            return Json(val, semantic_tag::none, alloc);
        }
    };

    // array back insertable

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    detail::is_compatible_array_type<Json,T>::value &&
                                                    ext_traits::is_back_insertable<T>::value 
                                                    >::type>
    {
        typedef typename std::iterator_traits<typename T::iterator>::value_type value_type;
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            bool result = j.is_array();
            if (result)
            {
                for (auto e : j.array_range())
                {
                    if (!e.template is<value_type>())
                    {
                        result = false;
                        break;
                    }
                }
            }
            return result;
        }

        // array back insertable non-byte container

        template <typename Container = T>
        static typename std::enable_if<!ext_traits::is_byte<typename Container::value_type>::value,result_type>::type
        try_as(const Json& j)
        {
            if (!j.is_array())
            {
                return result_type(jsoncons::unexpect, conv_errc::not_vector);
            }
            T result;
            visit_reserve_(typename std::integral_constant<bool, ext_traits::has_reserve<T>::value>::type(),result,j.size());
            for (const auto& item : j.array_range())
            {
                auto res = item.template try_as<value_type>();
                if (JSONCONS_UNLIKELY(!res))
                {
                    return result_type(jsoncons::unexpect, res.error().code(), res.error().message_arg());
                }
                result.push_back(std::move(*res));
            }

            return result_type(std::move(result));
        }

        // array back insertable byte container

        template <typename Container = T>
        static typename std::enable_if<ext_traits::is_byte<typename Container::value_type>::value,result_type>::type
        try_as(const Json& j)
        {
            std::error_code ec;
            if (j.is_array())
            {
                T result;
                visit_reserve_(typename std::integral_constant<bool, ext_traits::has_reserve<T>::value>::type(),result,j.size());
                for (const auto& item : j.array_range())
                {
                    auto res = item.template try_as<value_type>();
                    if (JSONCONS_UNLIKELY(!res))
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_vector);
                    }
                    result.push_back(std::move(*res));
                }

                return result_type(std::move(result));
            }
            else if (j.is_byte_string_view())
            {
                value_converter<byte_string_view,T> converter;
                auto v = converter.convert(j.as_byte_string_view(),j.tag(), ec);
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(ec);
                }
                return v;
            }
            else if (j.is_string())
            {
                value_converter<basic_string_view<char>,T> converter;
                auto v = converter.convert(j.as_string_view(),j.tag(), ec);
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(ec);
                }
                return v;
            }
            else
            {
                return result_type(jsoncons::unexpect, conv_errc::not_vector);
            }
        }

        template <typename Container = T>
        static typename std::enable_if<!ext_traits::is_std_byte<typename Container::value_type>::value,Json>::type
        to_json(const T& val)
        {
            Json j(json_array_arg);
            auto first = std::begin(val);
            auto last = std::end(val);
            std::size_t size = std::distance(first,last);
            j.reserve(size);
            for (auto it = first; it != last; ++it)
            {
                j.push_back(*it);
            }
            return j;
        }

        template <typename Container = T>
        static typename std::enable_if<!ext_traits::is_std_byte<typename Container::value_type>::value,Json>::type
        to_json(const T& val, const allocator_type& alloc)
        {
            Json j(json_array_arg, alloc);
            auto first = std::begin(val);
            auto last = std::end(val);
            std::size_t size = std::distance(first, last);
            j.reserve(size);
            for (auto it = first; it != last; ++it)
            {
                j.push_back(*it);
            }
            return j;
        }

        template <typename Container = T>
        static typename std::enable_if<ext_traits::is_std_byte<typename Container::value_type>::value,Json>::type
        to_json(const T& val)
        {
            Json j(byte_string_arg, val);
            return j;
        }

        template <typename Container = T>
        static typename std::enable_if<ext_traits::is_std_byte<typename Container::value_type>::value,Json>::type
        to_json(const T& val, const allocator_type& alloc)
        {
            Json j(byte_string_arg, val, semantic_tag::none, alloc);
            return j;
        }

        static void visit_reserve_(std::true_type, T& v, std::size_t size)
        {
            v.reserve(size);
        }

        static void visit_reserve_(std::false_type, T&, std::size_t)
        {
        }
    };

    // array, not back insertable but insertable

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    detail::is_compatible_array_type<Json,T>::value &&
                                                    !ext_traits::is_back_insertable<T>::value &&
                                                    ext_traits::is_insertable<T>::value>::type>
    {
        typedef typename std::iterator_traits<typename T::iterator>::value_type value_type;
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            bool result = j.is_array();
            if (result)
            {
                for (auto e : j.array_range())
                {
                    if (!e.template is<value_type>())
                    {
                        result = false;
                        break;
                    }
                }
            }
            return result;
        }

        static result_type try_as(const Json& j)
        {
            if (j.is_array())
            {
                T result;
                for (const auto& item : j.array_range())
                {
                    auto res = item.template try_as<value_type>();
                    if (JSONCONS_UNLIKELY(!res))
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_vector);
                    }
                    result.insert(std::move(*res));
                }

                return result_type(std::move(result));
            }
            else 
            {
                return result_type(jsoncons::unexpect, conv_errc::not_vector);
            }
        }

        static Json to_json(const T& val)
        {
            Json j(json_array_arg);
            auto first = std::begin(val);
            auto last = std::end(val);
            std::size_t size = std::distance(first,last);
            j.reserve(size);
            for (auto it = first; it != last; ++it)
            {
                j.push_back(*it);
            }
            return j;
        }

        static Json to_json(const T& val, const allocator_type& alloc)
        {
            Json j(json_array_arg, alloc);
            auto first = std::begin(val);
            auto last = std::end(val);
            std::size_t size = std::distance(first, last);
            j.reserve(size);
            for (auto it = first; it != last; ++it)
            {
                j.push_back(*it);
            }
            return j;
        }
    };

    // array not back insertable or insertable, but front insertable

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    detail::is_compatible_array_type<Json,T>::value &&
                                                    !ext_traits::is_back_insertable<T>::value &&
                                                    !ext_traits::is_insertable<T>::value &&
                                                    ext_traits::is_front_insertable<T>::value>::type>
    {
        typedef typename std::iterator_traits<typename T::iterator>::value_type value_type;
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            bool result = j.is_array();
            if (result)
            {
                for (auto e : j.array_range())
                {
                    if (!e.template is<value_type>())
                    {
                        result = false;
                        break;
                    }
                }
            }
            return result;
        }

        static result_type try_as(const Json& j)
        {
            if (j.is_array())
            {
                T result;

                auto it = j.array_range().rbegin();
                auto end = j.array_range().rend();
                for (; it != end; ++it)
                {
                    auto res = (*it).template try_as<value_type>();
                    if (JSONCONS_UNLIKELY(!res))
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_vector);
                    }
                    result.push_front(std::move(*res));
                }

                return result_type(std::move(result));
            }
            else 
            {
                return result_type(jsoncons::unexpect, conv_errc::not_vector);
            }
        }

        static Json to_json(const T& val)
        {
            Json j(json_array_arg);
            auto first = std::begin(val);
            auto last = std::end(val);
            std::size_t size = std::distance(first,last);
            j.reserve(size);
            for (auto it = first; it != last; ++it)
            {
                j.push_back(*it);
            }
            return j;
        }

        static Json to_json(const T& val, const allocator_type& alloc)
        {
            Json j(json_array_arg, alloc);
            auto first = std::begin(val);
            auto last = std::end(val);
            std::size_t size = std::distance(first, last);
            j.reserve(size);
            for (auto it = first; it != last; ++it)
            {
                j.push_back(*it);
            }
            return j;
        }
    };

    // std::array

    template <typename Json,typename E, std::size_t N>
    struct json_conv_traits<Json, std::array<E, N>>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<std::array<E, N>>;

        using value_type = E;

        static bool is(const Json& j) noexcept
        {
            bool result = j.is_array() && j.size() == N;
            if (result)
            {
                for (auto e : j.array_range())
                {
                    if (!e.template is<value_type>())
                    {
                        result = false;
                        break;
                    }
                }
            }
            return result;
        }

        static result_type try_as(const Json& j)
        {
            std::array<E, N> buff;
            if (j.size() != N)
            {
                return result_type(jsoncons::unexpect, conv_errc::not_array);
            }
            for (std::size_t i = 0; i < N; i++)
            {
                auto res = j[i].template try_as<E>();
                if (JSONCONS_UNLIKELY(!res))
                {
                    return result_type(jsoncons::unexpect, conv_errc::not_array);
                }
                buff[i] = std::move(*res);
            }
            return result_type(std::move(buff));
        }

        static Json to_json(const std::array<E, N>& val)
        {
            Json j(json_array_arg);
            j.reserve(N);
            for (auto it = val.begin(); it != val.end(); ++it)
            {
                j.push_back(*it);
            }
            return j;
        }

        static Json to_json(const std::array<E, N>& val, 
                            const allocator_type& alloc)
        {
            Json j(json_array_arg, alloc);
            j.reserve(N);
            for (auto it = val.begin(); it != val.end(); ++it)
            {
                j.push_back(*it);
            }
            return j;
        }
    };

    // map like
    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    ext_traits::is_map_like<T>::value &&
                                                    ext_traits::is_constructible_from_const_pointer_and_size<typename T::key_type>::value &&
                                                    is_json_conv_traits_specialized<Json,typename T::mapped_type>::value>::type
    >
    {
        using mapped_type = typename T::mapped_type;
        using value_type = typename T::value_type;
        using key_type = typename T::key_type;
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            bool result = j.is_object();
            for (auto member : j.object_range())
            {
                if (!member.value().template is<mapped_type>())
                {
                    result = false;
                }
            }
            return result;
        }

        static result_type try_as(const Json& j)
        {
            if (!j.is_object())
            {
                return result_type(jsoncons::unexpect, conv_errc::not_map);
            }
            T result;
            for (const auto& item : j.object_range())
            {
                auto res = item.value().template try_as<mapped_type>();
                if (!res)
                {
                    return result_type(jsoncons::unexpect, res.error().code(), res.error().message_arg());
                }
                result.emplace(key_type(item.key().data(),item.key().size()), std::move(*res));
            }

            return result_type(std::move(result));
        }

        static Json to_json(const T& val)
        {
            Json j(json_object_arg, val.begin(), val.end());
            return j;
        }

        static Json to_json(const T& val, const allocator_type& alloc)
        {
            Json j(json_object_arg, val.begin(), val.end(), alloc);
            return j;
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    ext_traits::is_map_like<T>::value &&
                                                    !ext_traits::is_constructible_from_const_pointer_and_size<typename T::key_type>::value &&
                                                    is_json_conv_traits_specialized<Json,typename T::key_type>::value &&
                                                    is_json_conv_traits_specialized<Json,typename T::mapped_type>::value>::type
    >
    {
        using mapped_type = typename T::mapped_type;
        using value_type = typename T::value_type;
        using key_type = typename T::key_type;
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& val) noexcept 
        {
            if (!val.is_object())
                return false;
            for (const auto& item : val.object_range())
            {
                Json j(item.key());
                if (!j.template is<key_type>())
                {
                    return false;
                }
                if (!item.value().template is<mapped_type>())
                {
                    return false;
                }
            }
            return true;
        }

        static result_type try_as(const Json& val) 
        {
            T result;
            for (const auto& item : val.object_range())
            {
                Json j(item.key());
                auto r = json_conv_traits<Json,key_type>::try_as(j);
                if (!r)
                {
                    return result_type(r.error());
                }
                result.emplace(std::move(r.value()), item.value().template as<mapped_type>());
            }

            return result_type(std::move(result));
        }

        static Json to_json(const T& val) 
        {
            Json j(json_object_arg);
            j.reserve(val.size());
            for (const auto& item : val)
            {
                auto temp = json_conv_traits<Json,key_type>::to_json(item.first);
                if (temp.is_string_view())
                {
                    j.try_emplace(typename Json::key_type(temp.as_string_view()), item.second);
                }
                else
                {
                    typename Json::key_type key;
                    temp.dump(key);
                    j.try_emplace(std::move(key), item.second);
                }
            }
            return j;
        }

        static Json to_json(const T& val, const allocator_type& alloc) 
        {
            Json j(json_object_arg, semantic_tag::none, alloc);
            j.reserve(val.size());
            for (const auto& item : val)
            {
                auto temp = json_conv_traits<Json, key_type>::to_json(item.first, alloc);
                if (temp.is_string_view())
                {
                    j.try_emplace(typename Json::key_type(temp.as_string_view(), alloc), item.second);
                }
                else
                {
                    typename Json::key_type key(alloc);
                    temp.dump(key);
                    j.try_emplace(std::move(key), item.second, alloc);
                }
            }
            return j;
        }
    };

    namespace tuple_detail
    {
        template<size_t Pos, std::size_t Size,typename Json,typename Tuple>
        struct json_tuple_helper
        {
            using element_type = typename std::tuple_element<Size-Pos, Tuple>::type;
            using next = json_tuple_helper<Pos-1, Size, Json, Tuple>;
            
            static bool is(const Json& j) noexcept
            {
                if (j[Size-Pos].template is<element_type>())
                {
                    return next::is(j);
                }
                else
                {
                    return false;
                }
            }

            static void try_as(Tuple& tuple, const Json& j, std::error_code& ec)
            {
                auto res = j[Size-Pos].template try_as<element_type>();
                if (!res)
                {
                    ec = res.error().code();
                    return;
                }
                std::get<Size-Pos>(tuple) = *res;
                next::try_as(tuple, j, ec);
            }

            static void to_json(const Tuple& tuple, Json& j)
            {
                j.push_back(json_conv_traits<Json, element_type>::to_json(std::get<Size-Pos>(tuple)));
                next::to_json(tuple, j);
            }
        };

        template<size_t Size,typename Json,typename Tuple>
        struct json_tuple_helper<0, Size, Json, Tuple>
        {
            static bool is(const Json&) noexcept
            {
                return true;
            }

            static void try_as(Tuple&, const Json&, std::error_code&)
            {
            }

            static void to_json(const Tuple&, Json&)
            {
            }
        };
    } // namespace tuple_detail

    template <typename Json,typename... E>
    struct json_conv_traits<Json, std::tuple<E...>>
    {
    private:
        using helper = tuple_detail::json_tuple_helper<sizeof...(E), sizeof...(E), Json, std::tuple<E...>>;

    public:
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<std::tuple<E...>>;

        static bool is(const Json& j) noexcept
        {
            return helper::is(j);
        }
        
        static result_type try_as(const Json& j)
        {
            std::error_code ec;
            std::tuple<E...> val;
            helper::try_as(val, j, ec);
            if (ec)
            {
                return result_type(unexpect, ec);
            }
            return result_type(std::move(val));
        }
         
        static Json to_json(const std::tuple<E...>& val)
        {
            Json j(json_array_arg);
            j.reserve(sizeof...(E));
            helper::to_json(val, j);
            return j;
        }

        static Json to_json(const std::tuple<E...>& val,
                            const allocator_type& alloc)
        {
            Json j(json_array_arg, alloc);
            j.reserve(sizeof...(E));
            helper::to_json(val, j);
            return j;
        }
    };

    template <typename Json,typename T1,typename T2>
    struct json_conv_traits<Json, std::pair<T1,T2>>
    {
    public:
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<std::pair<T1,T2>>;

        static bool is(const Json& j) noexcept
        {
            return j.is_array() && j.size() == 2;
        }
        
        static result_type try_as(const Json& j)
        {
            if (!j.is_array() || j.size() != 2)
            {
                return result_type(unexpect, conv_errc::not_pair);
            }
            auto res1 = j[0].template try_as<T1>();
            if (!res1)
            {
                return result_type(unexpect, res1.error().code());
            }
            auto res2 = j[1].template try_as<T2>();
            if (!res2)
            {
                return result_type(unexpect, res2.error().code());
            }
            return result_type(std::make_pair<T1,T2>(std::move(*res1), std::move(*res2)));
        }
        
        static Json to_json(const std::pair<T1,T2>& val)
        {
            Json j(json_array_arg);
            j.reserve(2);
            j.push_back(val.first);
            j.push_back(val.second);
            return j;
        }

        static Json to_json(const std::pair<T1, T2>& val, const allocator_type& alloc)
        {
            Json j(json_array_arg, alloc);
            j.reserve(2);
            j.push_back(val.first);
            j.push_back(val.second);
            return j;
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T,
        typename std::enable_if<ext_traits::is_basic_byte_string<T>::value>::type>
    {
    public:
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.is_byte_string();
        }
        
        static result_type try_as(const Json& j)
        { 
            return j.template try_as_byte_string<typename T::allocator_type>();
        }
        
        static Json to_json(const T& val, 
                            const allocator_type& alloc = allocator_type())
        {
            return Json(byte_string_arg, val, semantic_tag::none, alloc);
        }
    };

    template <typename Json,typename ValueType>
    struct json_conv_traits<Json, std::shared_ptr<ValueType>,
        typename std::enable_if<!is_json_conv_traits_declared<std::shared_ptr<ValueType>>::value &&
                                !std::is_polymorphic<ValueType>::value
    >::type>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<std::shared_ptr<ValueType>>;

        static bool is(const Json& j) noexcept
        {
            return j.is_null() || j.template is<ValueType>();
        }

        static result_type try_as(const Json& j) 
        {
            if (j.is_null())
            {
                return result_type(std::shared_ptr<ValueType>(nullptr));
            }
            auto r = j.template try_as<ValueType>();
            if (!r)
            {
                return result_type(r.error());
            }
            return result_type(std::make_shared<ValueType>(std::move(r.value())));
        }

        static Json to_json(const std::shared_ptr<ValueType>& ptr, 
            const allocator_type& alloc = allocator_type())
        {
            if (ptr.get() != nullptr) 
            {
                Json j(*ptr, alloc);
                return j;
            }
            else 
            {
                return Json::null();
            }
        }
    };

    template <typename Json,typename ValueType>
    struct json_conv_traits<Json, std::unique_ptr<ValueType>,
                            typename std::enable_if<!is_json_conv_traits_declared<std::unique_ptr<ValueType>>::value &&
                                                    !std::is_polymorphic<ValueType>::value
    >::type>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<std::unique_ptr<ValueType>>;

        static bool is(const Json& j) noexcept
        {
            return j.is_null() || j.template is<ValueType>();
        }

        static result_type try_as(const Json& j) 
        {
            if (j.is_null())
            {
                return result_type(std::unique_ptr<ValueType>(nullptr));
            }
            auto r = j.template try_as<ValueType>();
            if (!r)
            {
                return result_type(r.error());
            }
            return result_type(jsoncons::make_unique<ValueType>(std::move(r.value())));
        }

        static Json to_json(const std::unique_ptr<ValueType>& ptr,
            const allocator_type& alloc = allocator_type())
        {
            if (ptr.get() != nullptr) 
            {
                Json j(*ptr, alloc);
                return j;
            }
            else 
            {
                return Json::null();
            }
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, jsoncons::optional<T>,
                            typename std::enable_if<!is_json_conv_traits_declared<jsoncons::optional<T>>::value>::type>
    {
        using allocator_type = typename Json::allocator_type;
    public:
        using result_type = conversion_result<jsoncons::optional<T>>;

        static bool is(const Json& j) noexcept
        {
            return j.is_null() || j.template is<T>();
        }
        
        static result_type try_as(const Json& j)
        { 
            if (j.is_null())
            {
                return result_type(jsoncons::optional<T>());
            }
            auto r = j.template try_as<T>();
            if (!r)
            {
                return result_type(r.error());
            }
            return result_type(jsoncons::optional<T>(std::move(r.value())));
        }
        
        static Json to_json(const jsoncons::optional<T>& val, 
            const allocator_type& alloc = allocator_type())
        {
            return val.has_value() ? Json(*val, alloc) : Json::null();
        }
    };

    template <typename Json>
    struct json_conv_traits<Json, byte_string_view>
    {
        using allocator_type = typename Json::allocator_type;

    public:
        using result_type = conversion_result<byte_string_view>;

        static bool is(const Json& j) noexcept
        {
            return j.is_byte_string_view();
        }
        
        static result_type try_as(const Json& j)
        {
            return result_type(j.try_as_byte_string_view());
        }
        
        static Json to_json(const byte_string_view& val, const allocator_type& alloc = allocator_type())
        {
            return Json(byte_string_arg, val, semantic_tag::none, alloc);
        }
    };

    // basic_bigint

    template <typename Json,typename Allocator>
    struct json_conv_traits<Json, basic_bigint<Allocator>>
    {
    public:
        using allocator_type = typename Json::allocator_type;
        using char_type = typename Json::char_type;
        using result_type = conversion_result<basic_bigint<Allocator>>;

        static bool is(const Json& j) noexcept
        {
            switch (j.type())
            {
                case json_type::string_value:
                    return jsoncons::utility::is_base10(j.as_string_view().data(), j.as_string_view().length());
                case json_type::int64_value:
                case json_type::uint64_value:
                    return true;
                default:
                    return false;
            }
        }
        
        static result_type try_as(const Json& j)
        {
            switch (j.type())
            {
                case json_type::string_value:
                {
                    auto sv = j.as_string_view();
                    std::error_code ec;
                    basic_bigint<Allocator> val;
                    auto r = to_bigint(sv.data(), sv.length(), val);
                    if (JSONCONS_UNLIKELY(!r))
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_bigint);
                    }
                    return result_type(std::move(val));
                }
                case json_type::half_value:
                case json_type::double_value:
                {
                    auto res = j.template try_as<int64_t>();
                    return res ? result_type(jsoncons::in_place, *res) : result_type(jsoncons::unexpect, conv_errc::not_bigint);
                }
                case json_type::int64_value:
                {
                    auto res = j.template try_as<int64_t>();
                    return res ? result_type(jsoncons::in_place, *res) : result_type(jsoncons::unexpect, conv_errc::not_bigint);
                }
                case json_type::uint64_value:
                {
                    auto res = j.template try_as<uint64_t>();
                    return res ? result_type(jsoncons::in_place, *res) : result_type(jsoncons::unexpect, conv_errc::not_bigint);
                }
                default:
                    return result_type(jsoncons::unexpect, conv_errc::not_bigint);
            }
        }
        
        static Json to_json(const basic_bigint<Allocator>& val)
        {
            std::basic_string<char_type> s;
            val.write_string(s);
            return Json(s,semantic_tag::bigint);
        }

        static Json to_json(const basic_bigint<Allocator>& val, const allocator_type&)
        {
            std::basic_string<char_type> s;
            val.write_string(s);
            return Json(s,semantic_tag::bigint);
        }
    };

    // std::valarray

    template <typename Json,typename T>
    struct json_conv_traits<Json, std::valarray<T>>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<std::valarray<T>>;

        static bool is(const Json& j) noexcept
        {
            bool result = j.is_array();
            if (result)
            {
                for (auto e : j.array_range())
                {
                    if (!e.template is<T>())
                    {
                        result = false;
                        break;
                    }
                }
            }
            return result;
        }
        
        static result_type try_as(const Json& j)
        {
            if (j.is_array())
            {
                std::valarray<T> v(j.size());
                for (std::size_t i = 0; i < j.size(); ++i)
                {
                    auto res = j[i].template try_as<T>();
                    if (JSONCONS_UNLIKELY(!res))
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_array);
                    }
                    v[i] = std::move(*res);
                }
                return result_type(std::move(v));
            }
            else
            {
                return result_type(jsoncons::unexpect, conv_errc::not_array);
            }
        }
        
        static Json to_json(const std::valarray<T>& val)
        {
            Json j(json_array_arg);
            auto first = std::begin(val);
            auto last = std::end(val);
            std::size_t size = std::distance(first,last);
            j.reserve(size);
            for (auto it = first; it != last; ++it)
            {
                j.push_back(*it);
            }
            return j;
        } 

        static Json to_json(const std::valarray<T>& val, const allocator_type& alloc)
        {
            Json j(json_array_arg, alloc);
            auto first = std::begin(val);
            auto last = std::end(val);
            std::size_t size = std::distance(first,last);
            j.reserve(size);
            for (auto it = first; it != last; ++it)
            {
                j.push_back(*it);
            }
            return j;
        }
    };

#if defined(JSONCONS_HAS_STD_VARIANT)

namespace variant_detail
{
    template<int N,typename Json,typename Variant,typename ... Args>
    typename std::enable_if<N == std::variant_size_v<Variant>, bool>::type
    is_variant(const Json& /*j*/)
    {
        return false;
    }

    template<std::size_t N,typename Json,typename Variant,typename T,typename ... U>
    typename std::enable_if<N < std::variant_size_v<Variant>, bool>::type
    is_variant(const Json& j)
    {
      if (j.template is<T>())
      {
          return true;
      }
      else
      {
          return is_variant<N+1, Json, Variant, U...>(j);
      }
    }

    template<int N,typename Json,typename Variant,typename ... Args>
    typename std::enable_if<N == std::variant_size_v<Variant>, conversion_result<Variant>>::type
    as_variant(const Json& /*j*/)
    {
        return conversion_result<Variant>(jsoncons::unexpect, conv_errc::not_variant);
    }

    template<std::size_t N,typename Json,typename Variant,typename T,typename ... U>
    typename std::enable_if<N < std::variant_size_v<Variant>, conversion_result<Variant>>::type
    as_variant(const Json& j)
    {
        using result_type = conversion_result<Variant>;
        if (j.template is<T>())
      {
          auto res = j.template try_as<T>();
          if (JSONCONS_UNLIKELY(!res))
          {
              return result_type(jsoncons::unexpect, conv_errc::not_variant);
          }
          return conversion_result<Variant>(jsoncons::in_place, std::move(*res));
      }
      else
      {
          return as_variant<N+1, Json, Variant, U...>(j);
      }
    }

    template <typename Json>
    struct variant_to_json_visitor
    {
        Json& j_;

        variant_to_json_visitor(Json& j) : j_(j) {}

        template <typename T>
        void operator()(const T& value) const
        {
            j_ = value;
        }
    };

} // namespace variant_detail

    template <typename Json,typename... VariantTypes>
    struct json_conv_traits<Json, std::variant<VariantTypes...>>
    {
    public:
        using variant_type = typename std::variant<VariantTypes...>;
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<std::variant<VariantTypes...>>;

        static bool is(const Json& j) noexcept
        {
            return variant_detail::is_variant<0,Json,variant_type, VariantTypes...>(j); 
        }

        static result_type try_as(const Json& j)
        {
            return result_type(variant_detail::as_variant<0,Json,variant_type, VariantTypes...>(j)); 
        }

        static Json to_json(const std::variant<VariantTypes...>& var)
        {
            Json j(json_array_arg);
            variant_detail::variant_to_json_visitor<Json> visitor(j);
            std::visit(visitor, var);
            return j;
        }

        static Json to_json(const std::variant<VariantTypes...>& var,
                            const allocator_type& alloc)
        {
            Json j(json_array_arg, alloc);
            variant_detail::variant_to_json_visitor<Json> visitor(j);
            std::visit(visitor, var);
            return j;
        }
    };
#endif

    // std::chrono::duration
    template <typename Json,typename Rep,typename Period>
    struct json_conv_traits<Json,std::chrono::duration<Rep,Period>>
    {
        using duration_type = std::chrono::duration<Rep,Period>;
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<duration_type>;

        static constexpr int64_t nanos_in_milli = 1000000;
        static constexpr int64_t nanos_in_second = 1000000000;
        static constexpr int64_t millis_in_second = 1000;

        static bool is(const Json& j) noexcept
        {
            return (j.tag() == semantic_tag::epoch_second || j.tag() == semantic_tag::epoch_milli || j.tag() == semantic_tag::epoch_nano);
        }

        static result_type try_as(const Json& j)
        {
            return from_json_(j);
        }

        static Json to_json(const duration_type& val, const allocator_type& = allocator_type())
        {
            return to_json_(val);
        }

        template <typename PeriodT=Period>
        static 
        typename std::enable_if<std::is_same<PeriodT,std::ratio<1>>::value, result_type>::type
        from_json_(const Json& j)
        {
            if (j.is_int64() || j.is_uint64() || j.is_double())
            {
                auto count = j.template as<Rep>();
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                        return result_type(in_place, count);
                    case semantic_tag::epoch_milli:
                        return result_type(in_place, count == 0 ? 0 : count/millis_in_second);
                    case semantic_tag::epoch_nano:
                        return result_type(in_place, count == 0 ? 0 : count/nanos_in_second);
                    default:
                        return result_type(in_place, count);
                }
            }
            else if (j.is_string())
            {
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                    {
                        auto res = j.template try_as<Rep>();
                        if (!res)
                        {
                            return result_type(unexpect, conv_errc::not_epoch);
                        }
                        return result_type(in_place, *res);
                    }
                    case semantic_tag::epoch_milli:
                    {
                        auto sv = j.as_string_view();
                        bigint n;
                        auto r = to_bigint(sv.data(), sv.length(), n);
                        if (!r) {return result_type(jsoncons::unexpect, conv_errc::not_epoch);}
                        if (n != 0)
                        {
                            n = n / millis_in_second;
                        }
                        return result_type(in_place, static_cast<Rep>(n));
                    }
                    case semantic_tag::epoch_nano:
                    {
                        auto sv = j.as_string_view();
                        bigint n;
                        auto r = to_bigint(sv.data(), sv.length(), n);
                        if (!r) {return result_type(jsoncons::unexpect, conv_errc::not_epoch);}
                        if (n != 0)
                        {
                            n = n / nanos_in_second;
                        }
                        return result_type(in_place, static_cast<Rep>(n));
                    }
                    default:
                    {
                        auto res = j.template try_as<Rep>();
                        if (!res)
                        {
                            return result_type(unexpect, conv_errc::not_epoch);
                        }
                        return result_type(in_place, *res);
                    }
                }
            }
            else
            {
                return result_type(unexpect, conv_errc::not_epoch);
            }
        }

        template <typename PeriodT=Period>
        static 
        typename std::enable_if<std::is_same<PeriodT,std::milli>::value, result_type>::type
        from_json_(const Json& j)
        {
            if (j.is_int64() || j.is_uint64())
            {
                auto res = j.template try_as<Rep>();
                if (!res)
                {
                    return result_type(unexpect, conv_errc::not_epoch);
                }
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                        return result_type(in_place, *res*millis_in_second);
                    case semantic_tag::epoch_milli:
                        return result_type(in_place, *res);
                    case semantic_tag::epoch_nano:
                        return result_type(in_place, *res == 0 ? 0 : *res/nanos_in_milli);
                    default:
                        return result_type(in_place, *res);
                }
            }
            else if (j.is_double())
            {
                auto res = j.template try_as<double>();
                if (!res)
                {
                    return result_type(unexpect, conv_errc::not_epoch);
                }
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                        return result_type(in_place, static_cast<Rep>(*res * millis_in_second));
                    case semantic_tag::epoch_milli:
                        return result_type(in_place, static_cast<Rep>(*res));
                    case semantic_tag::epoch_nano:
                        return result_type(in_place, *res == 0 ? 0 : static_cast<Rep>(*res / nanos_in_milli));
                    default:
                        return result_type(in_place, static_cast<Rep>(*res));
                }
            }
            else if (j.is_string())
            {
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                    {
                        auto res = j.template try_as<Rep>();
                        if (!res)
                        {
                            return result_type(unexpect, conv_errc::not_epoch);
                        }
                        return result_type(in_place, *res*millis_in_second);
                    }
                    case semantic_tag::epoch_milli:
                    {
                        auto res = j.try_as_string_view();
                        if (!res)
                        {
                            return result_type(unexpect, conv_errc::not_epoch);
                        }
                        Rep n{0};
                        auto result = jsoncons::utility::dec_to_integer((*res).data(), (*res).size(), n);
                        if (!result)
                        {
                            return result_type(unexpect, conv_errc::not_epoch);
                        }
                        return result_type(in_place, n);
                    }
                    case semantic_tag::epoch_nano:
                    {
                        auto sv = j.as_string_view();
                        bigint n;
                        auto r = to_bigint(sv.data(), sv.length(), n);
                        if (!r) {return result_type(jsoncons::unexpect, conv_errc::not_epoch);}
                        if (n != 0)
                        {
                            n = n / nanos_in_milli;
                        }
                        return result_type(in_place, static_cast<Rep>(n));
                    }
                    default:
                    {
                        auto res = j.template try_as<Rep>();
                        if (!res)
                        {
                            return result_type(unexpect, conv_errc::not_epoch);
                        }
                        return result_type(in_place, *res);
                    }
                }
            }
            else
            {
                return result_type(unexpect, conv_errc::not_epoch);
            }
        }

        template <typename PeriodT=Period>
        static 
        typename std::enable_if<std::is_same<PeriodT,std::nano>::value, result_type>::type
        from_json_(const Json& j)
        {
            if (j.is_int64() || j.is_uint64() || j.is_double())
            {
                auto count = j.template as<Rep>();
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                        return result_type(in_place, count*nanos_in_second);
                    case semantic_tag::epoch_milli:
                        return result_type(in_place, count*nanos_in_milli);
                    case semantic_tag::epoch_nano:
                        return result_type(in_place, count);
                    default:
                        return result_type(in_place, count);
                }
            }
            else if (j.is_double())
            {
                auto count = j.template as<double>();
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                        return result_type(in_place, static_cast<Rep>(count * nanos_in_second));
                    case semantic_tag::epoch_milli:
                        return result_type(in_place, static_cast<Rep>(count * nanos_in_milli));
                    case semantic_tag::epoch_nano:
                        return result_type(in_place, static_cast<Rep>(count));
                    default:
                        return result_type(in_place, static_cast<Rep>(count));
                }
            }
            else if (j.is_string())
            {
                auto res = j.template try_as<Rep>();
                if (!res)
                {
                    return result_type(unexpect, conv_errc::not_epoch);
                }
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                        return result_type(in_place, *res*nanos_in_second);
                    case semantic_tag::epoch_milli:
                        return result_type(in_place, *res*nanos_in_milli);
                    case semantic_tag::epoch_nano:
                        return result_type(in_place, *res);
                    default:
                        return result_type(in_place, *res);
                }
            }
            else
            {
                return result_type(unexpect, conv_errc::not_epoch);
            }
        }

        template <typename PeriodT=Period>
        static 
        typename std::enable_if<std::is_same<PeriodT,std::ratio<1>>::value,Json>::type
        to_json_(const duration_type& val)
        {
            return Json(val.count(), semantic_tag::epoch_second);
        }

        template <typename PeriodT=Period>
        static 
        typename std::enable_if<std::is_same<PeriodT,std::milli>::value,Json>::type
        to_json_(const duration_type& val)
        {
            return Json(val.count(), semantic_tag::epoch_milli);
        }

        template <typename PeriodT=Period>
        static 
        typename std::enable_if<std::is_same<PeriodT,std::nano>::value,Json>::type
        to_json_(const duration_type& val)
        {
            return Json(val.count(), semantic_tag::epoch_nano);
        }
    };

    // std::nullptr_t
    template <typename Json>
    struct json_conv_traits<Json,std::nullptr_t>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<std::nullptr_t>;

        static bool is(const Json& j) noexcept
        {
            return j.is_null();
        }

        static result_type try_as(const Json& j)
        {
            if (!j.is_null())
            {
                return result_type(jsoncons::unexpect, conv_errc::not_nullptr);
            }
            return result_type(nullptr);
        }

        static Json to_json(const std::nullptr_t&, const allocator_type& = allocator_type())
        {
            return Json::null();
        }
    };

    // std::bitset

    struct null_back_insertable_byte_container
    {
        using value_type = uint8_t;

        void push_back(value_type)
        {
        }
    };

    template <typename Json, std::size_t N>
    struct json_conv_traits<Json, std::bitset<N>>
    {
        using allocator_type = typename Json::allocator_type;
        using result_type = conversion_result<std::bitset<N>>;

        static bool is(const Json& j) noexcept
        {
            if (j.is_byte_string())
            {
                return true;
            }
            else if (j.is_string())
            {
                jsoncons::string_view sv = j.as_string_view();
                null_back_insertable_byte_container cont;
                auto result = base16_to_bytes(sv.begin(), sv.end(), cont);
                return result.ec == conv_errc::success ? true : false;
            }
            return false;
        }

        static result_type try_as(const Json& j)
        {
            if (j.template is<uint64_t>())
            {
                auto bits = j.template as<uint64_t>();
                std::bitset<N> bs = static_cast<unsigned long long>(bits);
                return result_type(std::move(bs));
            }
            else if (j.is_byte_string() || j.is_string())
            {
                std::bitset<N> bs;
                std::vector<uint8_t> bits;
                if (j.is_byte_string())
                {
                    bits = j.template as<std::vector<uint8_t>>();
                }
                else
                {
                    jsoncons::string_view sv = j.as_string_view();
                    auto result = base16_to_bytes(sv.begin(), sv.end(), bits);
                    if (result.ec != conv_errc::success)
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_bitset);
                    }
                }
                std::uint8_t byte = 0;
                std::uint8_t mask  = 0;

                std::size_t pos = 0;
                for (std::size_t i = 0; i < N; ++i)
                {
                    if (mask == 0)
                    {
                        if (pos >= bits.size())
                        {
                            return result_type(jsoncons::unexpect, conv_errc::not_bitset);
                        }
                        byte = bits.at(pos++);
                        mask = 0x80;
                    }

                    if (byte & mask)
                    {
                        bs[i] = 1;
                    }

                    mask = static_cast<std::uint8_t>(mask >> 1);
                }
                return result_type(std::move(bs));
            }
            else
            {
                return result_type(jsoncons::unexpect, conv_errc::not_bitset);
            }
        }

        static Json to_json(const std::bitset<N>& val, 
                            const allocator_type& alloc = allocator_type())
        {
            std::vector<uint8_t> bits;

            uint8_t byte = 0;
            uint8_t mask = 0x80;

            for (std::size_t i = 0; i < N; ++i)
            {
                if (val[i])
                {
                    byte |= mask;
                }

                mask = static_cast<uint8_t>(mask >> 1);

                if (mask == 0)
                {
                    bits.push_back(byte);
                    byte = 0;
                    mask = 0x80;
                }
            }

            // Encode remainder
            if (mask != 0x80)
            {
                bits.push_back(byte);
            }

            Json j(byte_string_arg, bits, semantic_tag::base16, alloc);
            return j;
        }
    };

} // namespace reflect
} // namespace jsoncons

#endif // JSONCONS_REFLECT_JSON_CONV_TRAITS_HPP
