﻿#ifndef JLCXX_STL_HPP
#define JLCXX_STL_HPP

#include <vector>

#include "module.hpp"
#include "type_conversion.hpp"

namespace jlcxx
{

namespace detail
{

struct UnusedT {};

/// Replace T1 by UnusedT if T1 == T2, return T1 otherwise
template<typename T1, typename T2>
struct SkipIfSameAs
{
  using type = T1;
};

template<typename T>
struct SkipIfSameAs<T,T>
{
  using type = UnusedT;
};

template<typename T1, typename T2> using skip_if_same = typename SkipIfSameAs<T1,T2>::type;

}

namespace stl
{

class StlWrappers
{
private:
  StlWrappers(Module& mod);
  static std::unique_ptr<StlWrappers> m_instance;
  Module& m_stl_mod;
public:
  TypeWrapper<Parametric<TypeVar<1>>> vector;

  static void instantiate(Module& mod);
  static StlWrappers& instance();

  inline jl_module_t* module() const
  {
    return m_stl_mod.julia_module();
  }
};

StlWrappers& wrappers();

using stltypes = remove_duplicates<combine_parameterlists<combine_parameterlists<ParameterList
<
  bool,
  double,
  float,
  char,
  unsigned char,
  wchar_t,
  void*,
  std::string,
  std::wstring
>, fundamental_int_types>, fixed_int_types>>;

template<typename TypeWrapperT>
void wrap_common(TypeWrapperT& wrapped)
{
  using WrappedT = typename TypeWrapperT::type;
  using T = typename WrappedT::value_type;
  wrapped.method("cppsize", &WrappedT::size);
  wrapped.module().last_function().set_override_module(StlWrappers::instance().module());
  wrapped.method("resize", [] (WrappedT& v, const int_t s) { v.resize(s); });
  wrapped.module().last_function().set_override_module(StlWrappers::instance().module());
  wrapped.method("append", [] (WrappedT& v, jlcxx::ArrayRef<T> arr)
  {
    const std::size_t addedlen = arr.size();
    v.reserve(v.size() + addedlen);
    for(size_t i = 0; i != addedlen; ++i)
    {
      v.push_back(arr[i]);
    }
  });
  wrapped.module().last_function().set_override_module(StlWrappers::instance().module());
}

template<typename T>
struct WrapVectorImpl
{
  template<typename TypeWrapperT>
  static void wrap(TypeWrapperT&& wrapped)
  {
    using WrappedT = std::vector<T>;
    
    wrap_common(wrapped);
    wrapped.method("push_back", static_cast<void (WrappedT::*)(const T&)>(&WrappedT::push_back));
    wrapped.method("getindex", [] (const WrappedT& v, int_t i) -> typename WrappedT::const_reference { return v[i-1]; });
    wrapped.method("getindex", [] (WrappedT& v, int_t i) -> typename WrappedT::reference { return v[i-1]; });
    wrapped.method("setindex!", [] (WrappedT& v, const T& val, int_t i) { v[i-1] = val; });
  }
};

template<>
struct WrapVectorImpl<bool>
{
  template<typename TypeWrapperT>
  static void wrap(TypeWrapperT&& wrapped)
  {
    using WrappedT = std::vector<bool>;

    wrap_common(wrapped);
    wrapped.method("push_back", [] (WrappedT& v, const bool val) { v.push_back(val); });
    wrapped.method("getindex", [] (const WrappedT& v, int_t i) { return bool(v[i-1]); });
    wrapped.method("setindex!", [] (WrappedT& v, const bool val, int_t i) { v[i-1] = val; });
  }
};

struct WrapVector
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using T = typename WrappedT::value_type;
    WrapVectorImpl<T>::wrap(wrapped);
  }
};

template<typename T>
inline void apply_stl(jlcxx::Module& mod)
{
  TypeWrapper<Parametric<TypeVar<1>>>(mod, StlWrappers::instance().vector).apply<std::vector<T>>(WrapVector());
}

}

}

#endif