cmake_minimum_required(VERSION 3.10)
project(YadroVadPlugin CXX)

set(CMAKE_CXX_STANDARD 17)

# Твой путь к GStreamer
set(GST_ROOT "C:/msvc_x86_64")

include_directories(
    "${GST_ROOT}/include/gstreamer-1.0"
    "${GST_ROOT}/include/glib-2.0"
    "${GST_ROOT}/lib/glib-2.0/include"
)

link_directories("${GST_ROOT}/lib")

add_library(gstyadrovad SHARED src/gst_yadro_vad.cpp)

# Линкуем библиотеки (имена файлов в папке lib без расширения .lib)
target_link_libraries(gstyadrovad 
    gstreamer-1.0 
    gstbase-1.0 
    gobject-2.0 
    glib-2.0
)