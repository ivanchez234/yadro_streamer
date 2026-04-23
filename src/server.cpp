// --- ВОЛШЕБНАЯ СТРОКА ДЛЯ WINDOWS ---
// Отключает загрузку старой сетевой библиотеки winsock.h во избежание конфликтов
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <iostream>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <set>

// --- ОТКЛЮЧАЕМ BOOST И ИСПОЛЬЗУЕМ СТАНДАРТНЫЙ C++ ---
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
// Сетевые библиотеки
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> WsServer;
typedef websocketpp::connection_hdl connection_hdl;

// Глобальные переменные для сети
WsServer echo_server;
std::set<connection_hdl, std::owner_less<connection_hdl>> connections;
std::mutex connections_mutex;

// --- СОБЫТИЯ WEBSOCKET СЕРВЕРА ---

void on_open(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(connections_mutex);
    connections.insert(hdl);
    std::cout << "[СЕТЬ] 🟢 Новый клиент подключился! Всего клиентов: " << connections.size() << std::endl;
}

void on_close(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(connections_mutex);
    connections.erase(hdl);
    std::cout << "[СЕТЬ] 🔴 Клиент отключился. Всего клиентов: " << connections.size() << std::endl;
}

// Функция для запуска WebSocket сервера в отдельном потоке
void run_websocket_server() {
    try {
        echo_server.set_access_channels(websocketpp::log::alevel::none); // Отключаем лишние логи
        echo_server.init_asio();
        
        echo_server.set_open_handler(&on_open);
        echo_server.set_close_handler(&on_close);
        
        echo_server.listen(8080); // Слушаем порт 8080
        echo_server.start_accept();
        
        std::cout << "[СЕТЬ] 🚀 WebSocket сервер запущен на ws://localhost:8080" << std::endl;
        echo_server.run();
    } catch (const std::exception & e) {
        std::cerr << "[СЕТЬ] Ошибка: " << e.what() << std::endl;
    }
}

// --- СОБЫТИЕ GSTREAMER (ПЕРЕХВАТ ЗВУКА) ---

static GstFlowReturn on_new_sample(GstElement *sink, gpointer user_data) {
    GstSample *sample;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    
    if (sample) {
        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        gst_buffer_map(buffer, &map, GST_MAP_READ);

        // БЛОК ОТПРАВКИ ДАННЫХ КЛИЕНТАМ
        {
            std::lock_guard<std::mutex> lock(connections_mutex);
            // Если есть хоть один подключенный клиент
            if (!connections.empty()) {
                // Рассылаем байты звука всем!
                for (auto it : connections) {
                    echo_server.send(it, map.data, map.size, websocketpp::frame::opcode::binary);
                }
                std::cout << "[СЕРВЕР] Разослано " << map.size << " байт аудио " << connections.size() << " клиентам." << std::endl;
            }
        }

        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }
    return GST_FLOW_ERROR;
}

// --- ОСНОВНАЯ ПРОГРАММА ---

int main(int argc, char *argv[]) {
    // Включаем поддержку UTF-8 в консоли Windows
    SetConsoleOutputCP(CP_UTF8);
    
    // Подсказываем GStreamer'у, где искать нашу скомпилированную DLL (плагин)
    _putenv_s("GST_PLUGIN_PATH", "C:\\code\\yadro_streamer\\build\\Debug");

    // Запускаем сеть в ФОНОВОМ потоке, чтобы она не мешала GStreamer
    std::thread ws_thread(run_websocket_server);

    gst_init(&argc, &argv);
    std::cout << "==== YADRO STREAMING SERVER: WEBSOCKET MODE ====" << std::endl;

    // Сборка пайплайна (Обрати внимание на sync=true в appsink)
    const char* pipeline_str = 
        "filesrc location=\"C:/code/yadro_streamer/test3.mp3\" ! "
        "decodebin ! audioconvert ! audioresample ! "
        "yadrovad vad-mode=3 hangover-time=200 ! "
        "audioconvert ! appsink name=mysink emit-signals=true sync=true";

    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(pipeline_str, &error);

    if (error) {
        std::cerr << "❌ Ошибка парсинга пайплайна: " << error->message << std::endl;
        g_clear_error(&error);
        exit(-1);
    }

    // Подключаем перехватчик (нашу функцию on_new_sample) к элементу appsink
    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), NULL);
    gst_object_unref(appsink);

    // Небольшая пауза, чтобы WebSocket успел стартануть перед звуком
    Sleep(1000); 

    std::cout << "▶️ Запуск аудио-движка..." << std::endl;
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    GstBus *bus = gst_element_get_bus(pipeline);
    
    // Блокируем главный поток, пока файл не закончится или не вылетит ошибка
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // Выясняем, почему аудио остановилось
    if (msg != nullptr) {
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError *err = nullptr; gchar *debug = nullptr;
            gst_message_parse_error(msg, &err, &debug);
            std::cerr << "\n❌ ОШИБКА АУДИО-ДВИЖКА: " << err->message << std::endl;
            g_clear_error(&err); g_free(debug);
        } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
            std::cout << "\n✅ Трек полностью завершен (Конец файла)." << std::endl;
        }
        gst_message_unref(msg);
    }

    std::cout << "🛑 Остановка пайплайна..." << std::endl;
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_object_unref(bus);

    std::cout << "🛑 Остановка сети..." << std::endl;
    echo_server.stop_listening();
    for (auto it : connections) { 
        echo_server.close(it, websocketpp::close::status::normal, ""); 
    }
    echo_server.stop();
    ws_thread.join(); // Ждем завершения сетевого потока

    std::cout << "\nСервер завершил работу. Нажмите Enter..." << std::endl;
    std::cin.get(); 

    return 0;
}