includes("lib/commonlibf4")

set_project("PlayerVoiceFrequency")
set_version("1.0.0")
set_license("MIT")
set_languages("c++23")
set_warnings("allextra")

add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")
add_defines("COMMONLIB_RUNTIMECOUNT=3")

target("PlayerVoiceFrequency")
    add_rules("commonlibf4.plugin", {
        name = "PlayerVoiceFrequency",
        author = "jarari",
        description = "Keeps the player voice frequency independent of time scaling",
        plugin_template = path.join(os.projectdir(), "res/commonlibf4-plugin.cpp.in"),
    })
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/PCH.h")
