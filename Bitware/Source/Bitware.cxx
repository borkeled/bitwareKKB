#include <iostream>
#include <thread>

#include "Driver/Driver.h"
#include "Globals.hxx"
#include "Miscellaneous/Output/Output.h"
#include "Core/Graphics/Graphics.h"
#include "Core/Application.h"
#include "Core/UI/UIBridge.h"
#include "Miscellaneous/Protection/External/oxorany_include.h"

// NOT used
#include "Auth/skStr.h"
#include <ShlObj.h>
#pragma comment(lib, "Shell32.lib")

std::string tm_to_readable_time(tm ctx);
static std::time_t string_to_timet(std::string timestamp);
static std::tm timet_to_tm(time_t timestamp);
const std::string compilation_date = (std::string)skCrypt(__DATE__);
const std::string compilation_time = (std::string)skCrypt(__TIME__);
void sessionStatus();

std::int32_t main(std::int32_t argc, char** argv[])
{
    Application app;

    if (!app.Init())
    {
        Api::MessageBoxA(nullptr, WRAPPER_MARCO("Failed to initialize"), WRAPPER_MARCO("Error"), MB_ICONERROR | MB_OK);
        return 1;
    }

    app.Run();

    return 0;
}

std::string tm_to_readable_time(tm ctx) {
    char buffer[80];
    strftime(buffer, sizeof(buffer), WRAPPER_MARCO("%a %m/%d/%y %H:%M:%S %Z"), &ctx);
    return std::string(buffer);
}

static std::time_t string_to_timet(std::string timestamp) {
    auto cv = strtol(timestamp.c_str(), NULL, 10);
    return (time_t)cv;
}

static std::tm timet_to_tm(time_t timestamp) {
    std::tm context;
    localtime_s(&context, &timestamp);
    return context;
}
