#include <gst/gst.h>
#include <glib.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <stdio.h>

#include <iostream>
#include <lib.hh>
#include <vector>
#include <string.h>
size_t MAX_DATAFRAME_LEN = 1024; //5120;

enum NETWORK_PACKET
{
    START_FRAME,
    END_FRAME
};

typedef struct _CustomData
{
    uvgrtp::media_stream *rtp_stream;
} CustomData;

GstFlowReturn new_sample(GstAppSink *appsink, CustomData *customData)
{
    printf("CAPTURE FRAME\n");
    static int framecount = 0;
    framecount++;

    static int width = 0, height = 0;

    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstCaps *caps = gst_sample_get_caps(sample);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    static GstStructure *s;
    const GstStructure *info = gst_sample_get_info(sample);

    // ---- Read frame and convert to opencv format ---------------
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    printf("size: %lld\n", map.size);
    // SOCKET
    // Send frame header
    uint8_t data = NETWORK_PACKET::START_FRAME;
    customData->rtp_stream->push_frame((uint8_t *)(&data), 1, RTP_NO_FLAGS);

    // Send chunks of the frame
    int sent = 0;
    while (map.size - sent > MAX_DATAFRAME_LEN)
    {
        customData->rtp_stream->push_frame((uint8_t *)(map.data + sent), MAX_DATAFRAME_LEN, RTP_NO_FLAGS);
        sent += MAX_DATAFRAME_LEN;
    }
    customData->rtp_stream->push_frame((uint8_t *)(map.data + sent), map.size - sent, RTP_NO_FLAGS);

    // Send frame ending
    data = NETWORK_PACKET::END_FRAME;
    customData->rtp_stream->push_frame((uint8_t *)(&data), 1, RTP_NO_FLAGS);

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

int main(int argc, char *argv[])
{
    CustomData socketData;
    GstElement *pipeline;
    GstElement *appsrc;
    GstBus *bus;
    GstMessage *msg;
    // SOCKET
    std::string hostname("127.0.0.1");
    auto receive_port = 8888;
    auto send_port = 8889;

    uvgrtp::context ctx;
    printf("Initializing the server\n");
    uvgrtp::session *session = ctx.create_session(hostname.c_str());
    printf("Initializing the server\n");
    // checkout RCC_UDP_SND_BUF_SIZE and RCC_UDP_RCV_BUF_SIZE
    // https://github.com/ultravideo/uvgRTP/issues/76
    uvgrtp::media_stream *rtp_stream = session->create_stream(receive_port, send_port, RTP_FORMAT_GENERIC, RCE_FRAGMENT_GENERIC);
    socketData.rtp_stream = rtp_stream;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Build the pipeline */
#ifdef _WIN32
    auto pipeline_args = "dx9screencapsrc width=1920 height=1080 ! video/x-raw,framerate=60/1 ! appsink name=sink";
    // auto pipeline_args = "dx9screencapsrc x=100 y=100 width=320 height=240 ! videoconvert ! autovideosink";
#else
    auto pipeline_args = "ximagesrc startx=1280 use-damage=0 ! video/x-raw,framerate=30/1 ! videoscale method=0 ! video/x-raw,width=640,height=480  ! ximagesink";
#endif
    pipeline = gst_parse_launch(pipeline_args, NULL);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    gst_app_sink_set_emit_signals((GstAppSink *)sink, true);
    gst_app_sink_set_drop((GstAppSink *)sink, true);
    gst_app_sink_set_max_buffers((GstAppSink *)sink, 1);
    GstAppSinkCallbacks callbacks = {NULL, NULL, (GstFlowReturn(__cdecl *)(GstAppSink *, gpointer))new_sample};
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &callbacks, &socketData, NULL);

    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                     (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
    {
        GError *err;
        gchar *debug;

        gst_message_parse_error(msg, &err, &debug);
        g_print("Error: %s\n", err->message);
        g_error_free(err);
        g_free(debug);

        break;
    }
    case GST_MESSAGE_EOS:
        /* end-of-stream */
        break;
    default:
        /* unhandled message */
        break;
    }

    /* Free resources */
    if (msg != NULL)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
