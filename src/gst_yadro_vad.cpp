#include "gst_yadro_vad.h"

GST_DEBUG_CATEGORY_STATIC (gst_yadro_vad_debug);
#define GST_CAT_DEFAULT gst_yadro_vad_debug

#define VAD_CAPS "audio/x-raw, format=S16LE, rate=16000, channels=1"
#define CHUNK_SIZE_BYTES 960 // Ровно 30 мс аудио

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(VAD_CAPS)
);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(VAD_CAPS)
);

G_DEFINE_TYPE (GstYadroVad, gst_yadro_vad, GST_TYPE_BASE_TRANSFORM);

/* Вызывается при запуске пайплайна (создаем адаптер) */
static gboolean gst_yadro_vad_start(GstBaseTransform *trans) {
    GstYadroVad *filter = GST_YADRO_VAD(trans);
    filter->adapter = gst_adapter_new();
    GST_INFO_OBJECT(filter, "YADRO VAD filter started. Adapter created.");
    return TRUE;
}

/* Вызывается при остановке (чистим память) */
static gboolean gst_yadro_vad_stop(GstBaseTransform *trans) {
    GstYadroVad *filter = GST_YADRO_VAD(trans);
    if (filter->adapter) {
        g_object_unref(filter->adapter);
        filter->adapter = NULL;
    }
    return TRUE;
}

/* ШАГ 1: Принимаем случайные куски аудио и складываем в адаптер */
static GstFlowReturn gst_yadro_vad_submit_input_buffer(GstBaseTransform *trans, gboolean is_discont, GstBuffer *input) {
    GstYadroVad *filter = GST_YADRO_VAD(trans);
    
    // Если произошел разрыв потока (перемотка), чистим старые данные
    if (is_discont) {
        gst_adapter_clear(filter->adapter);
    }
    
    // Пушим буфер в накопитель. GStreamer сам освободит память input.
    gst_adapter_push(filter->adapter, input);
    
    return GST_FLOW_OK;
}

/* ШАГ 2: Формируем исходящие данные строгими блоками */
static GstFlowReturn gst_yadro_vad_generate_output(GstBaseTransform *trans, GstBuffer **outbuf) {
    GstYadroVad *filter = GST_YADRO_VAD(trans);
    gsize available = gst_adapter_available(filter->adapter);

    // Если накопилось меньше 30 мс (960 байт), говорим пайплайну: "Ждем еще"
    if (available < CHUNK_SIZE_BYTES) {
        *outbuf = NULL; 
        return GST_FLOW_OK;
    }

    // Берем только количество байт, кратное 960 (чтобы не отрезать куски фрейма)
    gsize bytes_to_take = (available / CHUNK_SIZE_BYTES) * CHUNK_SIZE_BYTES;
    
    // Достаем этот ровный кусок и отдаем дальше
    *outbuf = gst_adapter_take_buffer(filter->adapter, bytes_to_take);

    return GST_FLOW_OK;
}

static void gst_yadro_vad_class_init(GstYadroVadClass *klass) {
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS(klass);

    gst_element_class_add_static_pad_template(element_class, &src_template);
    gst_element_class_add_static_pad_template(element_class, &sink_template);
    
    gst_element_class_set_static_metadata(element_class,
        "YADRO VAD Filter", "Filter/Effect/Audio",
        "Removes silence from audio stream (Stage 2: Chunk Resizing)", "Developer");

    // Привязываем наши новые функции
    trans_class->start = GST_DEBUG_FUNCPTR(gst_yadro_vad_start);
    trans_class->stop = GST_DEBUG_FUNCPTR(gst_yadro_vad_stop);
    trans_class->submit_input_buffer = GST_DEBUG_FUNCPTR(gst_yadro_vad_submit_input_buffer);
    trans_class->generate_output = GST_DEBUG_FUNCPTR(gst_yadro_vad_generate_output);
}

static void gst_yadro_vad_init(GstYadroVad *filter) {
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(filter), FALSE);
}

static gboolean plugin_init(GstPlugin *plugin) {
    GST_DEBUG_CATEGORY_INIT(gst_yadro_vad_debug, "yadrovad", 0, "Yadro VAD plugin");
    return gst_element_register(plugin, "yadrovad", GST_RANK_NONE, GST_TYPE_YADRO_VAD);
}

#define PACKAGE "yadrovad"
GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR, GST_VERSION_MINOR, yadrovad, "YADRO Voice Activity Detection",
    plugin_init, "1.0", "LGPL", "YadroStreamer", "http://yadro.com"
)