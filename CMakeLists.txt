cmake_minimum_required(VERSION 3.10)
project(YadroVadPlugin C CXX) # Включили поддержку чистого Си!
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)      # Стандарт для нейронки VAD

set(GST_ROOT "C:/msvc_x86_64")

# Жесткий абсолютный путь к текущей папке проекта
set(VAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/libfvad")

include_directories(
    "${GST_ROOT}/include/gstreamer-1.0"
    "${GST_ROOT}/include/glib-2.0"
    "${GST_ROOT}/lib/glib-2.0/include"
    "${VAD_DIR}/include"
    "${VAD_DIR}/src"
)

link_directories("${GST_ROOT}/lib")

# Ищем файлы РЕКУРСИВНО во всех подпапках
file(GLOB_RECURSE VAD_SOURCES 
    "${VAD_DIR}/src/*.c"
)

# Считаем и выводим количество найденных файлов (Обязательно проверь это в консоли!)
list(LENGTH VAD_SOURCES VAD_FILES_COUNT)
message(STATUS "=== ВНИМАНИЕ: Найдено файлов нейронки VAD: ${VAD_FILES_COUNT} ===")

add_library(gstyadrovad SHARED src/gst_yadro_vad.cpp ${VAD_SOURCES})

target_link_libraries(gstyadrovad 
    gstreamer-1.0 
    gstbase-1.0 
    gobject-2.0 
    glib-2.0
)

# --- СБОРКА СЕРВЕРА ---
# Создаем исполняемый файл из server.cpp
add_executable(yadro_server src/server.cpp)

# Подключаем к серверу библиотеки GStreamer
target_link_libraries(yadro_server gstreamer-1.0 gobject-2.0 glib-2.0)