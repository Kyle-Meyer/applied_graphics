///============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  Brian Russin
//	File:    logging.hpp
//	Purpose: Write messages to a log file
//============================================================================

#ifndef __COMMON_LOGGING_HPP__
#define __COMMON_LOGGING_HPP__

#include <cstdint>
#include <string>

namespace cg
{

void init_logging(const std::string &filename);

void log_msg(const char *message, ...);

class Log
{
  public:
    Log(const Log &) = delete;
    Log &operator=(const Log &) = delete;

    static Log               &get_instance();
    static const std::string &get_filename();

    void set_filename(const std::string &filename);

  private:
    Log();
    std::string filename_;
};

} // namespace cg

#endif
