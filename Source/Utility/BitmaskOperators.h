#pragma once

#include <type_traits>

//—ñ‹“Œ^‚²‚Æ‚Éƒrƒbƒg‰‰Z‚ğ‹–‰Â‚·‚é‚©‚Ç‚¤‚©‚ğØ‚è‘Ö‚¦‚é‚½‚ß‚Ì¯•Ê—pƒgƒŒƒCƒg
template <typename T>
struct EnableBitmaskOperators : std::false_type {};

//OR‰‰Z
template <typename E>
constexpr std::enable_if_t<EnableBitmaskOperators<E>::value, E>
operator|(E lhs, E rhs) noexcept
{
    using Underlying = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs));
}

//AND‰‰Z
template <typename E>
constexpr std::enable_if_t<EnableBitmaskOperators<E>::value, E>
operator&(E lhs, E rhs) noexcept
{
    using Underlying = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs));
}

//NOT‰‰Z
template <typename E>
constexpr std::enable_if_t<EnableBitmaskOperators<E>::value, E>
operator~(E val) noexcept
{
    using Underlying = std::underlying_type_t<E>;
    return static_cast<E>(~static_cast<Underlying>(val));
}

//’Ç‰Á
template <typename E>
constexpr std::enable_if_t<EnableBitmaskOperators<E>::value, E&>
operator|=(E& lhs, E rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

//íœ
template <typename E>
constexpr std::enable_if_t<EnableBitmaskOperators<E>::value, E&>
operator&=(E& lhs, E rhs) noexcept
{
    lhs = lhs & rhs;
    return lhs;
}