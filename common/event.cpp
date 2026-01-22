#include "common/event.hpp"

namespace cg
{

bool operator|(EventType lhs, EventType rhs)
{
    return (static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)) != 0;
}

EventType &operator|=(EventType &lhs, EventType rhs)
{
    lhs = static_cast<EventType>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
    return lhs;
}

bool operator&(EventType lhs, EventType rhs)
{
    return (static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)) != 0;
}

EventType &operator&=(EventType &lhs, EventType rhs)
{
    lhs = static_cast<EventType>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
    return lhs;
}

bool operator^(EventType lhs, EventType rhs)
{
    return (static_cast<uint32_t>(lhs) ^ static_cast<uint32_t>(rhs)) != 0;
}

EventType &operator^=(EventType &lhs, EventType rhs)
{
    lhs = static_cast<EventType>(static_cast<uint32_t>(lhs) ^ static_cast<uint32_t>(rhs));
    return lhs;
}

} // namespace cg
