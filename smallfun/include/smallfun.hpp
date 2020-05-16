#ifndef SMALLFUNCTION_SMALLFUNCTION_HPP
#define SMALLFUNCTION_SMALLFUNCTION_HPP

#include <type_traits>
#include <cinttypes>
#include <utility>

namespace smallfun
{
enum class Methods
{
  Copy, Move, Both
};

template<class Signature, std::size_t Size = 64, std::size_t Align = std::max(alignof(std::intptr_t), alignof(double)), Methods methods = Methods::Copy>
class function;


template<typename T>
struct not_function : std::true_type { };

template<class S, std::size_t Sz, std::size_t Al, Methods M>
struct not_function<function<S, Sz, Al, M>> : std::false_type { };

template<class R, class...Xs, std::size_t Size, std::size_t Align>
class function<R(Xs...), Size, Align, Methods::Copy>
{
  using call_operator = R(*)(void* self, Xs...);
  using copy_operator = void(*)(void* self, void*);
  using dest_operator = void(*)(void* self);

  call_operator vtbl_call{};
  alignas(Align) char m_memory[Size];
  copy_operator vtbl_copy;
  dest_operator vtbl_dest;

public:
  function() = default;

  template<class F, typename = std::enable_if_t<not_function<std::remove_cv_t<std::remove_reference_t<F>>>::value>>
  function(F&& f)
  {
    using T = std::remove_cv_t<std::remove_reference_t<F>>;

    static_assert(alignof(T) <= Align, "alignment must be increased");
    static_assert(sizeof(T) <= Size, "argument too large for SmallFun");
    new (m_memory) T{std::forward<F>(f)};
    vtbl_copy = [] (void* self, void* memory)
    {
      new (memory) T{*reinterpret_cast<T*>(self)};
    };
    vtbl_call = [] (void* self, Xs... xs) -> R
    {
      return (*reinterpret_cast<T*>(self))(xs...);
    };
    vtbl_dest = [] (void* self)
    {
      (*reinterpret_cast<T*>(self)).~T();
    };
  }

  template<class S, std::size_t Sz, std::size_t Al, Methods M>
  function(const function<S, Sz, Al, M>& sf) = delete;
  template<class S, std::size_t Sz, std::size_t Al, Methods M>
  function(function<S, Sz, Al, M>&& sf) = delete;
  template<class R2, class...Xs2, std::size_t Sz, std::size_t Al, Methods M>
  function(const function<R2(Xs2...), Sz, Al, M>& sf) = delete;
  template<class R2, class...Xs2, std::size_t Sz, std::size_t Al, Methods M>
  function(function<R2(Xs2...), Sz, Al, M>&& sf) = delete;

  function(function&& sf)
    : vtbl_call{sf.vtbl_call}
    , vtbl_copy{sf.vtbl_copy}
    , vtbl_dest{sf.vtbl_dest}
  {
    if(allocated())
      sf.copy(m_memory);
  }

  function(const function& sf)
    : vtbl_call{sf.vtbl_call}
    , vtbl_copy{sf.vtbl_copy}
    , vtbl_dest{sf.vtbl_dest}
  {
    if(allocated())
      sf.copy(m_memory);
  }

  function& operator=(const function& sf)
  {
    clean();
    vtbl_call = sf.vtbl_call;
    vtbl_copy = sf.vtbl_copy;
    vtbl_dest = sf.vtbl_dest;
    if(allocated())
      sf.copy(m_memory);
    return *this;
  }

  ~function()
  {
    if (allocated())
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

  bool allocated() const noexcept { return bool(vtbl_call); }
private:
  void clean()
  {
    if (allocated())
    {
      destruct();
      vtbl_call = nullptr;
    }
  }

  void copy(void* data) const
  {
    if (allocated())
    {
      return vtbl_copy((void*)m_memory, data);
    }
  }

  void destruct()
  {
    return vtbl_dest((void*)m_memory);
  }
};


template<class R, class...Xs, std::size_t Size, std::size_t Align>
class function<R(Xs...), Size, Align, Methods::Move>
{
  using call_operator = R(*)(void* self, Xs...);
  using move_operator = void(*)(void* self, void*);
  using dest_operator = void(*)(void* self);

  call_operator vtbl_call{};
  alignas(Align) char m_memory[Size];
  move_operator vtbl_move;
  dest_operator vtbl_dest;

public:
  function() = default;

  template<class F, typename = std::enable_if_t<not_function<std::remove_cv_t<std::remove_reference_t<F>>>::value>>
  function(F&& f)
  {
    using T = std::remove_cv_t<std::remove_reference_t<F>>;
    static_assert(alignof(T) <= Align, "alignment must be increased");
    static_assert(sizeof(T) <= Size, "argument too large for SmallFun");
    new (m_memory) T{std::forward<F>(f)};
    vtbl_move = [] (void* self, void* memory)
    {
      new (memory) T{std::move(*reinterpret_cast<T*>(self))};
    };
    vtbl_call = [] (void* self, Xs... xs) -> R
    {
      return (*reinterpret_cast<F*>(self))(xs...);
    };
    vtbl_dest = [] (void* self)
    {
      (*reinterpret_cast<T*>(self)).~T();
    };
  }

  template<class S, std::size_t Sz, std::size_t Al, Methods M>
  function(const function<S, Sz, Al, M>& sf) = delete;
  template<class S, std::size_t Sz, std::size_t Al, Methods M>
  function(function<S, Sz, Al, M>&& sf) = delete;

  function(function&& sf)
    : vtbl_call{sf.vtbl_call}
    , vtbl_move{sf.vtbl_move}
    , vtbl_dest{sf.vtbl_dest}
  {
    if(allocated())
      sf.move(m_memory);
  }

  function(const function&& sf) = delete;
  function& operator=(const function& sf) = delete;

  function& operator=(function&& sf)
  {
    clean();
    vtbl_call = sf.vtbl_call;
    vtbl_move = sf.vtbl_move;
    vtbl_dest = sf.vtbl_dest;

    if(allocated())
      sf.move(m_memory);
    return *this;
  }

  ~function()
  {
    if (allocated())
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

  bool allocated() const noexcept { return bool(vtbl_call); }
private:
  void clean()
  {
    if (allocated())
    {
      destruct();
      vtbl_call = nullptr;
    }
  }

  void move(void* data)
  {
    if (allocated())
    {
      vtbl_move((void*)m_memory, data);
      destruct();
      vtbl_call = nullptr;
    }
  }

  void destruct()
  {
    return vtbl_dest((void*)m_memory);
  }
};

template<class R, class...Xs, std::size_t Size, std::size_t Align>
class function<R(Xs...), Size, Align, Methods::Both>
{
  using call_operator = R(*)(void* self, Xs...);
  using copy_operator = void(*)(void* self, void*);
  using move_operator = void(*)(void* self, void*);
  using dest_operator = void(*)(void* self);

  call_operator vtbl_call{};
  alignas(Align) char m_memory[Size];
  copy_operator vtbl_copy;
  move_operator vtbl_move;
  dest_operator vtbl_dest;

public:
  function() = default;

  template<class F, typename = std::enable_if_t<not_function<std::remove_cv_t<std::remove_reference_t<F>>>::value>>
  function(F&& f)
  {
    using T = std::remove_cv_t<std::remove_reference_t<F>>;
    static_assert(alignof(T) <= Align, "alignment must be increased");
    static_assert(sizeof(T) <= Size, "argument too large for SmallFun");
    new (m_memory) T{std::forward<F>(f)};
    vtbl_copy = [] (void* self, void* memory)
    {
        new (memory) T{*reinterpret_cast<T*>(self)};
    };
    vtbl_move = [] (void* self, void* memory)
    {
      new (memory) T{std::move(*reinterpret_cast<T*>(self))};
    };
    vtbl_call = [] (void* self, Xs... xs) -> R
    {
      return (*reinterpret_cast<F*>(self))(xs...);
    };
    vtbl_dest = [] (void* self)
    {
      (*reinterpret_cast<T*>(self)).~T();
    };
  }

  template<class S, std::size_t Sz, std::size_t Al, Methods M>
  function(const function<S, Sz, Al, M>& sf) = delete;
  template<class S, std::size_t Sz, std::size_t Al, Methods M>
  function(function<S, Sz, Al, M>&& sf) = delete;

  function(const function& sf)
    : vtbl_call{sf.vtbl_call}
    , vtbl_copy{sf.vtbl_copy}
    , vtbl_move{sf.vtbl_move}
    , vtbl_dest{sf.vtbl_dest}
  {
    if(allocated())
      sf.copy(m_memory);
  }

  function(function&& sf)
    : vtbl_call{sf.vtbl_call}
    , vtbl_copy{sf.vtbl_copy}
    , vtbl_move{sf.vtbl_move}
    , vtbl_dest{sf.vtbl_dest}
  {
    if(allocated())
      sf.move(m_memory);
  }

  function& operator=(const function& sf)
  {
    clean();
    vtbl_call = sf.vtbl_call;
    vtbl_copy = sf.vtbl_copy;
    vtbl_move = sf.vtbl_move;
    vtbl_dest = sf.vtbl_dest;

    if(allocated())
      sf.copy(m_memory);
    return *this;
  }

  function& operator=(function&& sf)
  {
    clean();
    vtbl_call = sf.vtbl_call;
    vtbl_copy = sf.vtbl_copy;
    vtbl_move = sf.vtbl_move;
    vtbl_dest = sf.vtbl_dest;

    if(allocated())
      sf.move(m_memory);
    return *this;
  }

  ~function()
  {
    if (allocated())
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

  bool allocated() const noexcept { return bool(vtbl_call); }

private:
  void clean()
  {
    if (allocated())
    {
      destruct();
      vtbl_call = nullptr;
    }
  }

  void copy(void* data) const
  {
    if (allocated())
    {
      return vtbl_copy((void*)m_memory, data);
    }
  }

  void move(void* data)
  {
    if (allocated())
    {
      vtbl_move((void*)m_memory, data);
      destruct();
      vtbl_call = nullptr;
    }
  }

  void destruct()
  {
    return vtbl_dest((void*)m_memory);
  }

};

}
#endif
