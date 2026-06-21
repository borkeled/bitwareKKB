#include <windows.h>
#include <cstdio>
#include <ctime>
#include <string>
#include <utility>
#include <Miscellaneous/Protection/External/oxorany_include.h>
namespace Output
{
    inline void Enable_Vt()
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(h, &mode);
        mode |= 0x0004;
        SetConsoleMode(h, mode);
    }
    inline std::string Timestamp()
    {
        std::time_t t = std::time(nullptr);
        std::tm bt{};
        localtime_s(&bt, &t);
        char buf[16]{};
        std::snprintf(buf, sizeof(buf), WRAPPER_MARCO("[%02d:%02d:%02d]"), bt.tm_hour, bt.tm_min, bt.tm_sec);
        return std::string(buf);
    }
    inline void Write_Prefix(const char* color, const char* tag)
    {
        auto ts = Timestamp();
        std::printf(WRAPPER_MARCO("%s %s[%s]\033[0m "), ts.c_str(), color, tag);
    }
    template<typename... A>
    inline void Info(const char* format, A&&... args)
    {
        Enable_Vt();
        Write_Prefix(WRAPPER_MARCO("\033[36m"), WRAPPER_MARCO("INFO"));
        std::printf(format, std::forward<A>(args)...);
        std::printf(WRAPPER_MARCO("\n"));
    }
    template<typename... A>
    inline void Warning(const char* format, A&&... args)
    {
        Enable_Vt();
        Write_Prefix(WRAPPER_MARCO("\033[33m"), WRAPPER_MARCO("WARNING"));
        std::printf(format, std::forward<A>(args)...);
        std::printf(WRAPPER_MARCO("\n"));
    }
    template<typename... A>
    inline void Error(const char* format, A&&... args)
    {
        Enable_Vt();
        Write_Prefix(WRAPPER_MARCO("\033[31m"), WRAPPER_MARCO("ERROR"));
        std::printf(format, std::forward<A>(args)...);
        std::printf(WRAPPER_MARCO("\n"));
    }
    template<typename... A>
    inline void Success(const char* format, A&&... args)
    {
        Enable_Vt();
        Write_Prefix(WRAPPER_MARCO("\033[32m"), WRAPPER_MARCO("SUCCESS"));
        std::printf(format, std::forward<A>(args)...);
        std::printf(WRAPPER_MARCO("\n"));
    }
    template<typename... A>
    inline void Bitware(const char* format, A&&... args)
    {
        Enable_Vt();
        Write_Prefix(WRAPPER_MARCO("\033[38;2;0;255;0m"), WRAPPER_MARCO("BITWARE.FUN"));
        std::printf(format, std::forward<A>(args)...);
        std::printf(WRAPPER_MARCO("\n"));
    }
}
