#include <gst/gst.h>
#include <iostream>
#include <windows.h> // Добавили для работы с Windows API
#include <stdlib.h>  // Добавили для _putenv_s

int main(int argc, char *argv[]) {
    // 1. ЧИНИМ КОДИРОВКУ: Заставляем консоль Windows понимать UTF-8 и эмодзи
    SetConsoleOutputCP(CP_UTF8);

    // 2. ЧИНИМ ДВОЙНОЙ КЛИК: Говорим программе, где лежит наш плагин прямо из кода
    // (Обрати внимание на двойные слеши \\ для путей в C++)
    _putenv_s("GST_PLUGIN_PATH", "C:\\code\\yadro_streamer\\build\\Debug");

    gst_init(&argc, &argv);
    std::cout << "==== YADRO STREAMING SERVER STARTED ====" << std::endl;

    const char* pipeline_str = 
        "filesrc location=\"C:/code/yadro_streamer/test1.mp3\" ! "
        "decodebin ! audioconvert ! audioresample ! "
        "yadrovad vad-mode=3 hangover-time=200 ! "
        "audioconvert ! directsoundsink";

    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(pipeline_str, &error);

    if (error) {
        std::cerr << "❌ Ошибка сборки пайплайна: " << error->message << std::endl;
        g_clear_error(&error);
        
        // Пауза перед выходом при ошибке
        std::cout << "\nНажмите Enter для выхода..." << std::endl;
        std::cin.get();
        return -1;
    }

    std::cout << "▶️ Запуск потока аудио..." << std::endl;
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    GstBus *bus = gst_element_get_bus(pipeline);
    std::cout << "⏳ Воспроизведение идет... (Закрой окно крестиком для отмены)" << std::endl;
    
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg != nullptr) {
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError *err;
            gchar *debug_info;
            gst_message_parse_error(msg, &err, &debug_info);
            std::cerr << "❌ Ошибка от элемента " << GST_OBJECT_NAME(msg->src) << ": " << err->message << std::endl;
            g_clear_error(&err);
            g_free(debug_info);
        } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
            std::cout << "✅ Конец файла успешно достигнут (EOS)." << std::endl;
        }
        gst_message_unref(msg);
    }

    std::cout << "🛑 Остановка пайплайна и очистка памяти..." << std::endl;
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_object_unref(bus);

    // 3. ПАУЗА В КОНЦЕ: Окно не закроется, пока ты не нажмешь Enter
    std::cout << "\nСервер завершил работу. Нажмите Enter для выхода..." << std::endl;
    std::cin.get(); 

    return 0;
}