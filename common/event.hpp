///============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  Brian Russin
//	File:    event.hpp
//	Purpose: Event types
//============================================================================

#ifndef __COMMON_EVENT_HPP__
#define __COMMON_EVENT_HPP__

#include <cstdint>
#include <string>

namespace cg
{

enum class EventType
{
    NONE = 0x0000,
    EXIT = 0x0001,
    REDRAW = 0x0002
};

bool operator|(EventType lhs, EventType rhs);

EventType &operator|=(EventType &lhs, EventType rhs);

bool operator&(EventType lhs, EventType rhs);

EventType &operator&=(EventType &lhs, EventType rhs);

bool operator^(EventType lhs, EventType rhs);

EventType &operator^=(EventType &lhs, EventType rhs);

} // namespace cg

#endif
