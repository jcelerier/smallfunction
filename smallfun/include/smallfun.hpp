#ifndef SMALLFUNCTION_SMALLFUNCTION_HPP
#define SMALLFUNCTION_SMALLFUNCTION_HPP

#include <type_traits>
#include <cinttypes>
#include <utility>

namespace smallfun
{
#if defined(__EMSCRIPTEN__)
template<class Signature, std::size_t Size = 64, std::size_t Align = 2 * sizeof(std::intptr_t)>
#else
template<class Signature, std::size_t Size = 64, std::size_t Align = sizeof(std::intptr_t)>
#endif
class function;

template<class R, class...Xs, std::size_t Size, std::size_t Align>
class function<R(Xs...), Size, Align>
{
  using call_operator = R(*)(void* self, Xs...);
  using copy_operator = void(*)(void* self, void*);
  using dest_operator = void(*)(void* self);

  call_operator vtbl_call;
  alignas(Align) char m_memory[Size];
  copy_operator vtbl_copy;
  dest_operator vtbl_dest;
  bool m_allocated{};

public:
  function() = default;

  template<class F>
  function(F f)
    : m_allocated{true}
  {
    static_assert(alignof(F) <= Align, "alignment must be increased");
    static_assert(sizeof(F) <= Size, "argument too large for SmallFun");
    new (m_memory) F{std::forward<F>(f)};
    vtbl_copy = [] (void* self, void* memory)
    {
      new (memory) F{*reinterpret_cast<F*>(self)};
    };
    vtbl_call = [] (void* self, Xs... xs) -> R
    {
      return (*reinterpret_cast<F*>(self))(xs...);
    };
    vtbl_dest = [] (void* self)
    {
      (*reinterpret_cast<F*>(self)).~F();
    };
  }

  template<typename... Args>
  function(function<Args...>&& sf) = delete;

  function(function&& sf)
    : vtbl_call{sf.vtbl_call}
    , vtbl_copy{sf.vtbl_copy}
    , vtbl_dest{sf.vtbl_dest}
    , m_allocated{sf.m_allocated}
  {
    sf.copy(m_memory);
  }

  function(const function& sf)
    : vtbl_call{sf.vtbl_call}
    , vtbl_copy{sf.vtbl_copy}
    , vtbl_dest{sf.vtbl_dest}
    , m_allocated{sf.m_allocated}
  {
    sf.copy(m_memory);
  }

  function& operator=(const function& sf)
  {
    clean();
    vtbl_call = sf.vtbl_call;
    vtbl_copy = sf.vtbl_copy;
    vtbl_dest = sf.vtbl_dest;
    sf.copy(m_memory);
    m_allocated = sf.m_allocated;
    return *this;
  }

  ~function()
  {
    if (m_allocated)
    {
      destruct();
    }
  }

  template<class...Ys>
  R operator()(Ys&&...ys)
  {
    return vtbl_call((void*)m_memory, std::forward<Ys>(ys)...);
  }

  template<class...Ys>
  R operator()(Ys&&...ys) const
  {
    return vtbl_call((void*)m_memory, std::forward<Ys>(ys)...);
  }

private:
  void clean()
  {
    if (m_allocated)
    {
      destruct();
      m_allocated = false;
    }
  }

  void copy(void* data) const
  {
    if (m_allocated)
    {
      return vtbl_copy((void*)m_memory, data);
    }
  }

  void destruct()
  {
    return vtbl_dest((void*)m_memory);
  }
};

}

#endif
