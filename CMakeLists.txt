cmake_minimum_required(VERSION 3.16)
project(MusicPlayer LANGUAGES CXX)

add_executable(MusicPlayer
    main.cpp
    Scripts/Commands/Commands.cpp
    Scripts/Data/FilesContainer.cpp
    Scripts/Data/Saver.cpp
    Scripts/Logic/Player.cpp
    Scripts/Presentation/Visuals.cpp
)
target_compile_features(MusicPlayer PRIVATE cxx_std_20)
target_include_directories(MusicPlayer PRIVATE . external)
if(UNIX AND NOT APPLE)
    target_link_libraries(MusicPlayer PRIVATE dl pthread m)
endif()
