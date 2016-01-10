
solution "task"
   filename "task"
   location "projects"
   configurations { "Debug", "Release" }
   platforms { "Win32", "macosx" }

   filter "platforms:Win32"
      system "Windows"
      architecture "x32"

project "task"
   kind "ConsoleApp"
   language "C++"
   targetdir( "build" )
   includedirs { "task" }
   -- vpaths { ["include"] = "**.h" }
   -- vpaths { ["source"] = "**.cpp" }

   files { "task/**.h", "test/**.cpp" }
   
   filter "configurations:Debug"
      flags { "Symbols" }
      defines { "_DEBUG" }

   filter "configurations:Release"
      flags { "Optimize" }
      defines { "NDEBUG" }
