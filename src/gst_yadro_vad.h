#ifndef __GST_YADRO_VAD_H__
#define __GST_YADRO_VAD_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/base/gstadapter.h> // Добавили адаптер

G_BEGIN_DECLS

#define GST_TYPE_YADRO_VAD (gst_yadro_vad_get_type())
G_DECLARE_FINAL_TYPE(GstYadroVad, gst_yadro_vad, GST, YADRO_VAD, GstBaseTransform)

struct _GstYadroVad {
    GstBaseTransform element;
    
    // Накопитель входящих байтов
    GstAdapter *adapter;
};

G_END_DECLS

#endif /* __GST_YADRO_VAD_H__ */