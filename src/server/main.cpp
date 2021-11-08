#include <gst/gst.h>
#include <glib.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <stdio.h>

#include <iostream>
#include <lib.hh>
#include <vector>
#include <string.h>
#include <assert.h>
#include "pac_network.h"

#define GSTREAMER_CAPTURE 0

int main(int argc, char *argv[])
{
    printf("Initializing the server\n");

    std::string hostname("127.0.0.1");
    auto receive_port = 8888;
    auto send_port = 8889;

    // UVGRTP Setup
    uvgrtp::context ctx;
    uvgrtp::session *session = ctx.create_session(hostname.c_str());
    // checkout RCC_UDP_SND_BUF_SIZE and RCC_UDP_RCV_BUF_SIZE
    // https://github.com/ultravideo/uvgRTP/issues/76
    uvgrtp::media_stream *rtp_stream = session->create_stream(receive_port, send_port, RTP_FORMAT_GENERIC, RCE_FRAGMENT_GENERIC);

    // Gstreamer Setup
    gst_init(&argc, &argv);
#if GSTREAMER_CAPTURE

#ifdef _WIN32
    auto pipeline_args = "dxgiscreencapsrc width=1920 height=1080 cursor=1 ! video/x-raw,framerate=60/1 ! appsink name=sink";
#else
    auto pipeline_args = "ximagesrc startx=2560 endx=4479 starty=0 endy=1080 use-damage=0 ! video/x-raw,framerate=60/1 ! videoscale method=0 ! video/x-raw,width=1920,height=1080 ! appsink name=sink";
#endif
#else
    auto pipeline_args = "dxgiscreencapsrc width=1920 height=1080 cursor=1 ! video/x-raw,framerate=60/1 ! nvh264enc preset=low-latency-hp zerolatency=true max-bitrate=2000 ! queue ! rtph264pay ! udpsink host=127.0.0.1 port=9996";
#endif
    GstElement *pipeline = gst_parse_launch(pipeline_args, NULL);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");

    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Wait until error or EOS */
    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
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

    ctx.destroy_session(session);
    return 0;
}
