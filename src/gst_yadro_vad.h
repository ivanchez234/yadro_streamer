#ifndef __GST_YADRO_VAD_H__
#define __GST_YADRO_VAD_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/base/gstadapter.h>

extern "C" {
    #include <fvad.h>
}

G_BEGIN_DECLS

typedef enum {
    VAD_STATE_SILENCE,
    VAD_STATE_SPEECH,
    VAD_STATE_HANGOVER
} GstYadroVadState;

#define GST_TYPE_YADRO_VAD (gst_yadro_vad_get_type())
G_DECLARE_FINAL_TYPE(GstYadroVad, gst_yadro_vad, GST, YADRO_VAD, GstBaseTransform)

struct _GstYadroVad {
    GstBaseTransform element;
    GstAdapter *adapter;
    Fvad *vad_inst;
    
    /* Этап 4: Конечный автомат */
    GstYadroVadState state;
    int hangover_time_left_ms;

    /* Этап 5: Магия времени */
    GstClockTime original_time;      // Идеальное время оригинального файла
    GstClockTime total_dropped_time; // Сколько времени мы уже удалили
    gboolean need_discont;           // Нужно ли поставить флаг склейки
};

G_END_DECLS

#endif /* __GST_YADRO_VAD_H__ */