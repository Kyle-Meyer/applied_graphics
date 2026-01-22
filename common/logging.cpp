#include "common/logging.hpp"

#include <cstdarg>
#include <cstdio>
#include <iostream>

namespace cg
{

void init_logging(const std::string &filename) { Log::get_instance().set_filename(filename); }

void log_msg(const char *message, ...)
{
    const auto &filename = Log::get_filename();

    static FILE *lfile = NULL;
    if(lfile == NULL) { lfile = fopen(filename.c_str(), "w"); }

    std::va_list arg;
    va_start(arg, message);
    std::vfprintf(lfile, message, arg);
    std::putc('\n', lfile);
    std::fflush(lfile);
    va_end(arg);
}

Log::Log() : filename_("") {}

Log &Log::get_instance()
{
    static Log inst;
    return inst;
}

const std::string &Log::get_filename() { return get_instance().filename_; }

void Log::set_filename(const std::string &filename)
{
    if(!filename_.empty()) return;
    filename_ = filename;
}

} // namespace cg
