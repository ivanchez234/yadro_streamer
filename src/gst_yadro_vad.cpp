#include "gst_yadro_vad.h"
#include <string.h> // Для memcpy

GST_DEBUG_CATEGORY_STATIC (gst_yadro_vad_debug);
#define GST_CAT_DEFAULT gst_yadro_vad_debug

/* Жестко фиксируем форматы на входе (sink) и выходе (src) */
#define VAD_CAPS "audio/x-raw, format=S16LE, rate=16000, channels=1"

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(VAD_CAPS)
);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(VAD_CAPS)
);

G_DEFINE_TYPE (GstYadroVad, gst_yadro_vad, GST_TYPE_BASE_TRANSFORM);

/* Главная функция обработки: вызывается для каждого куска аудио */
static GstFlowReturn gst_yadro_vad_transform(GstBaseTransform *trans, GstBuffer *inbuf, GstBuffer *outbuf) {
    GstMapInfo in_map, out_map;

    // Получаем доступ к памяти входящего и исходящего буферов
    gst_buffer_map(inbuf, &in_map, GST_MAP_READ);
    gst_buffer_map(outbuf, &out_map, GST_MAP_WRITE);

    // Этап 1: Просто копируем данные без изменений (Passthrough)
    memcpy(out_map.data, in_map.data, in_map.size);

    // Освобождаем память
    gst_buffer_unmap(outbuf, &out_map);
    gst_buffer_unmap(inbuf, &in_map);

    return GST_FLOW_OK;
}

/* Инициализация класса (привязка функций и метаданных) */
static void gst_yadro_vad_class_init(GstYadroVadClass *klass) {
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS(klass);

    gst_element_class_add_static_pad_template(element_class, &src_template);
    gst_element_class_add_static_pad_template(element_class, &sink_template);
    
    gst_element_class_set_static_metadata(element_class,
        "YADRO VAD Filter", 
        "Filter/Effect/Audio",
        "Removes silence from audio stream (Stage 1: Passthrough)", 
        "Developer");

    // Переопределяем метод обработки
    trans_class->transform = GST_DEBUG_FUNCPTR(gst_yadro_vad_transform);
}

/* Инициализация экземпляра плагина */
static void gst_yadro_vad_init(GstYadroVad *filter) {
    // Отключаем встроенный passthrough, чтобы GStreamer принудительно 
    // вызывал нашу функцию transform и выделял outbuf
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(filter), FALSE);
}

/* Точка входа: регистрация плагина в системе GStreamer */
static gboolean plugin_init(GstPlugin *plugin) {
    GST_DEBUG_CATEGORY_INIT(gst_yadro_vad_debug, "yadrovad", 0, "Yadro VAD plugin");
    return gst_element_register(plugin, "yadrovad", GST_RANK_NONE, GST_TYPE_YADRO_VAD);
}

#define PACKAGE "yadrovad"

GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR, GST_VERSION_MINOR, yadrovad, "YADRO Voice Activity Detection",
    plugin_init, "1.0", "LGPL", "YadroStreamer", "http://yadro.com"
)