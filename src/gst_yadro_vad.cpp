#include "gst_yadro_vad.h"

GST_DEBUG_CATEGORY_STATIC (gst_yadro_vad_debug);
#define GST_CAT_DEFAULT gst_yadro_vad_debug

#define VAD_CAPS "audio/x-raw, format=S16LE, rate=16000, channels=1"
#define CHUNK_SIZE_BYTES 960

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(VAD_CAPS));
static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(VAD_CAPS));

G_DEFINE_TYPE (GstYadroVad, gst_yadro_vad, GST_TYPE_BASE_TRANSFORM);

static gboolean gst_yadro_vad_start(GstBaseTransform *trans) {
    GstYadroVad *filter = GST_YADRO_VAD(trans);
    filter->adapter = gst_adapter_new();
    filter->vad_inst = fvad_new();
    fvad_set_mode(filter->vad_inst, 3);
    fvad_set_sample_rate(filter->vad_inst, 16000);
    
    // Инициализируем счетчик
    filter->next_pts = 0; 
    
    GST_INFO_OBJECT(filter, "YADRO VAD filter started. VAD Mode: 3");
    return TRUE;
}

static gboolean gst_yadro_vad_stop(GstBaseTransform *trans) {
    GstYadroVad *filter = GST_YADRO_VAD(trans);
    if (filter->adapter) {
        g_object_unref(filter->adapter);
        filter->adapter = NULL;
    }
    if (filter->vad_inst) {
        fvad_free(filter->vad_inst);
        filter->vad_inst = NULL;
    }
    return TRUE;
}

static GstFlowReturn gst_yadro_vad_submit_input_buffer(GstBaseTransform *trans, gboolean is_discont, GstBuffer *input) {
    GstYadroVad *filter = GST_YADRO_VAD(trans);
    // Убрали очистку адаптера, теперь MP3 не будет обрываться
    gst_adapter_push(filter->adapter, input);
    return GST_FLOW_OK;
}

static GstFlowReturn gst_yadro_vad_generate_output(GstBaseTransform *trans, GstBuffer **outbuf) {
    GstYadroVad *filter = GST_YADRO_VAD(trans);
    gsize available = gst_adapter_available(filter->adapter);

    if (available < CHUNK_SIZE_BYTES) {
        *outbuf = NULL; 
        return GST_FLOW_OK;
    }

    GstBuffer *temp_buf = gst_adapter_take_buffer(filter->adapter, CHUNK_SIZE_BYTES);

    GstMapInfo map;
    gst_buffer_map(temp_buf, &map, GST_MAP_READ);

    const int16_t *samples = (const int16_t *)map.data;
    size_t num_samples = map.size / 2;

    int is_speech = fvad_process(filter->vad_inst, samples, num_samples);

    // Убрали эмодзи для стабильности консоли
    if (is_speech == 1) {
        GST_INFO_OBJECT(filter, "[VAD] SPEECH");
    } else if (is_speech == 0) {
        GST_INFO_OBJECT(filter, "[VAD] SILENCE");
    }

    gst_buffer_unmap(temp_buf, &map);
    
    // МАГИЯ ВРЕМЕНИ: Восстанавливаем метки времени для плеера!
    GST_BUFFER_PTS(temp_buf) = filter->next_pts;
    GST_BUFFER_DURATION(temp_buf) = 30 * GST_MSECOND; // 30 миллисекунд
    filter->next_pts += 30 * GST_MSECOND;

    *outbuf = temp_buf; 
    return GST_FLOW_OK;
}

static void gst_yadro_vad_class_init(GstYadroVadClass *klass) {
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS(klass);

    gst_element_class_add_static_pad_template(element_class, &src_template);
    gst_element_class_add_static_pad_template(element_class, &sink_template);
    gst_element_class_set_static_metadata(element_class,
        "YADRO VAD Filter", "Filter/Effect/Audio",
        "Removes silence from audio stream", "Developer");

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