workspace "MyProject"
   configurations { "Debug", "Release", "Distribution" }

project "MyProject"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "bin/%{cfg.buildcfg}"
   buildoptions { "-Wall", "-Wextra", "-Wpedantic" }

   files { "**.h", "**.cpp" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"
      optimize "Debug"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

   filter "configurations:Distribution"
      defines { "NDEBUG" }
      optimize "Full"