cmake_minimum_required(VERSION 3.10)

# Включаем поддержку C++ (для сервера и плагина) и чистого C (для нейронки)
project(YadroStreamer C CXX) 

# --- НАСТРОЙКИ СТАНДАРТОВ ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)      

# --- ГЛОБАЛЬНЫЕ ПУТИ GSTREAMER ---
set(GST_ROOT "C:/msvc_x86_64")

include_directories(
    "${GST_ROOT}/include/gstreamer-1.0"
    "${GST_ROOT}/include/glib-2.0"
    "${GST_ROOT}/lib/glib-2.0/include"
)
link_directories("${GST_ROOT}/lib")


# ==========================================
# ЧАСТЬ 1: СБОРКА ПЛАГИНА (gstyadrovad.dll)
# ==========================================

# Путь к исходникам нейронки
set(VAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/libfvad")

# Добавляем заголовочные файлы нейронки для плагина
include_directories(
    "${VAD_DIR}/include"
    "${VAD_DIR}/src"
)

# Ищем все .c файлы нейронки во всех подпапках
file(GLOB_RECURSE VAD_SOURCES "${VAD_DIR}/src/*.c")

# Выводим статистику в консоль при сборке
list(LENGTH VAD_SOURCES VAD_FILES_COUNT)
message(STATUS "=== Найдено файлов нейронки WebRTC VAD: ${VAD_FILES_COUNT} ===")

# Собираем DLL плагина
add_library(gstyadrovad SHARED src/gst_yadro_vad.cpp ${VAD_SOURCES})

# Линкуем базовые библиотеки GStreamer
target_link_libraries(gstyadrovad 
    gstreamer-1.0 
    gstbase-1.0 
    gobject-2.0 
    glib-2.0
)

<<<<<<< HEAD

# ==========================================
# ЧАСТЬ 2: СБОРКА СЕРВЕРА (yadro_server.exe)
# ==========================================

# Собираем исполняемый файл
add_executable(yadro_server src/server.cpp)

# Подключаем пути к сетевым библиотекам (ASIO и WebSocket++)
target_include_directories(yadro_server PRIVATE 
    "src/asio/asio/include"
    "src/asio/include"      # Добавили запасной путь!
    "src/websocketpp"
)

# ВАЖНО: Отключаем Boost и используем стандартный C++11 для веб-сокетов
target_compile_definitions(yadro_server PRIVATE 
    ASIO_STANDALONE 
    _WEBSOCKETPP_CPP11_STL_
)

# Линкуем GStreamer, плагин перехвата (app) и сетевые сокеты Windows (ws2_32)
target_link_libraries(yadro_server 
    gstreamer-1.0 
    gobject-2.0 
    glib-2.0 
    gstapp-1.0 
    ws2_32 
)
