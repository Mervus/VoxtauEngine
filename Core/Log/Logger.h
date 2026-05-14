//
// Created by Marvin on 14/05/2026.
//

#ifndef VOXTAU_LOGGER_H
#define VOXTAU_LOGGER_H

#include <iostream>
#include <cstdarg>
#include <cstdio>



class Log
{
public:
    enum LogState
    {
        ClientLog, ServerLog, UnknownLog
    };

public:
    Log() = default;
    Log(LogState logState) : state(logState) { };

    LogState state = UnknownLog;

    /**
     * Print a printf-style formatted message, prefixed with [CLIENT] or [SERVER].
     *
     * @param format printf-style format string (e.g. "x=%d name=%s").
     * @param ...    arguments matching the format specifiers.
     *
     * Output is truncated to 1024 bytes per call.
     */
    void Info(const char* format, ...) const
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        std::vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        std::cout << handleLogStateString() << buffer << std::endl;
    };

private:
    [[nodiscard]] std::string handleLogStateString() const
    {
        std::string res;
        switch (state)
        {
            case ClientLog: res = "{CLIENT} ";   break;
            case ServerLog: res = "{SERVER}: ";   break;
            case UnknownLog: res = "{UNKNOWN}: "; break;
        };

        return res;
    }
};

#endif //VOXTAU_LOGGER_H