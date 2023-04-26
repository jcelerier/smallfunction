#ifndef SMALLFUNCTION_TRIVIAL_HPP
#define SMALLFUNCTION_TRIVIAL_HPP

#include <smallfun.hpp>

namespace smallfun
{
template<class Signature, std::size_t Size = DefaultSize, std::size_t Align = DefaultAlign>
class trivial_function;

template<typename T>
struct not_trivial_function : std::true_type { };

template<class S, std::size_t Sz, std::size_t Al>
struct not_trivial_function<trivial_function<S, Sz, Al>> : std::false_type { };

template<typename F>
using simplified = std::remove_cv_t<std::remove_reference_t<F>>;

template<class R, class...Xs, std::size_t Size, std::size_t Align>
class trivial_function<R(Xs...), Size, Align>
{
  using call_operator = R(*)(void* self, Xs...);

  call_operator vtbl_call{};
  alignas(Align) char m_memory[Size * SMALLFUN_SIZE_MULTIPLIER];

public:

  template<class F>
    requires (std::is_trivially_copyable_v<simplified<F>> && not_trivial_function<simplified<F>>::value)
  constexpr trivial_function(F&& f) noexcept
  {
    using T = simplified<F>;

    static_assert(alignof(T) <= Align, "alignment must be increased");
    static_assert(sizeof(T) <= Size * SMALLFUN_SIZE_MULTIPLIER, "argument too large for trivial_function");
    memcpy(m_memory, &f, sizeof(F));
    vtbl_call = [] (void* self, Xs... xs) -> R {
      return (*reinterpret_cast<T*>(self))(xs...);
    };
  }

  template<class F>
    requires (std::is_trivially_copyable_v<simplified<F>> && not_trivial_function<simplified<F>>::value)
  constexpr trivial_function& operator=(F&& f) noexcept
  {
    using T = simplified<F>;

    static_assert(alignof(T) <= Align, "alignment must be increased");
    static_assert(sizeof(T) <= Size * SMALLFUN_SIZE_MULTIPLIER, "argument too large for trivial_function");
    memcpy(m_memory, &f, sizeof(F));
    vtbl_call = [] (void* self, Xs... xs) -> R {
      return (*reinterpret_cast<T*>(self))(xs...);
    };
    return *this;
  }

  template<class S, std::size_t Sz, std::size_t Al>
  trivial_function(const trivial_function<S, Sz, Al>& sf) = delete;
  template<class S, std::size_t Sz, std::size_t Al>
  trivial_function(trivial_function<S, Sz, Al>&& sf) = delete;
  template<class R2, class...Xs2, std::size_t Sz, std::size_t Al>
  trivial_function(const trivial_function<R2(Xs2...), Sz, Al>& sf) = delete;
  template<class R2, class...Xs2, std::size_t Sz, std::size_t Al>
  trivial_function(trivial_function<R2(Xs2...), Sz, Al>&& sf) = delete;

  constexpr trivial_function() noexcept = default;
  constexpr trivial_function(const trivial_function& sf) noexcept = default;
  constexpr trivial_function(trivial_function&& sf) noexcept = default;
  constexpr trivial_function& operator=(const trivial_function& sf) noexcept = default;
  constexpr trivial_function& operator=(trivial_function&& sf) noexcept = default;
  constexpr ~trivial_function() = default;

  template<class...Ys>
  constexpr R operator()(Ys&&...ys)
  {
    return vtbl_call((void*)m_memory, std::forward<Ys>(ys)...);
  }

  template<class...Ys>
  constexpr R operator()(Ys&&...ys) const
  {
    return vtbl_call((void*)m_memory, std::forward<Ys>(ys)...);
  }

  constexpr operator bool() const noexcept { return bool(vtbl_call); }
  constexpr bool allocated() const noexcept { return bool(vtbl_call); }
};
}
#endif
