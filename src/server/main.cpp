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

#define GSTREAMER_CAPTURE 1

typedef struct _CustomData
{
    uvgrtp::media_stream *rtp_stream;
} CustomData;

GstFlowReturn frame_recorded_callback(GstAppSink *appsink, CustomData *customData)
{
    // Pulling the frame from gstreamer
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo video_frame_info;
    gst_buffer_map(buffer, &video_frame_info, GST_MAP_READ);
    printf("size: %lld\n", video_frame_info.size);

    // SOCKET
    NetworkPacket video_network_packet = {
        NETWORK_PACKET_TYPE::VIDEO, // packet_type
        0,                          // buffer_offset
        NULL                        // data
    };

    int byte_size_to_send = sizeof(video_network_packet.data);
    while (byte_size_to_send > 0)
    {
        // TODO: we don't "need" this memcpy, when we can fit a whole encoded video frame into a single network frame
        // we can remove this (right now we need it because we want to send a chunk of the video frame
        // and some metadata like the packet type and buf_offset)
        memcpy(video_network_packet.data, video_frame_info.data + video_network_packet.buffer_offset, byte_size_to_send);
        customData->rtp_stream->push_frame((uint8_t *)&video_network_packet, sizeof(NetworkPacket), RTP_NO_FLAGS);
        video_network_packet.buffer_offset += byte_size_to_send;

        if (video_network_packet.buffer_offset + byte_size_to_send > video_frame_info.size)
        {
            // We don't have a full frame left to send
            byte_size_to_send = video_frame_info.size - video_network_packet.buffer_offset;
        }
    }
    video_network_packet.packet_type = NETWORK_PACKET_TYPE::FLUSH_SCREEN;
    customData->rtp_stream->push_frame((uint8_t *)&video_network_packet, sizeof(NetworkPacket), RTP_NO_FLAGS);

    gst_buffer_unmap(buffer, &video_frame_info);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

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
    auto pipeline_args = "ximagesrc startx=2560 starty=0 endx=4480 endy=1080 use-damage=0 ! video/x-raw,framerate=60/1 ! appsink name=sink";
#endif
#else
    auto pipeline_args = "videotestsrc ! video/x-raw,width=1920,height=1080,format=RGBx,framerate=60/1 ! appsink name=sink";
#endif
    GstElement *pipeline = gst_parse_launch(pipeline_args, NULL);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    gst_app_sink_set_emit_signals((GstAppSink *)sink, true);
    gst_app_sink_set_drop((GstAppSink *)sink, true);
    gst_app_sink_set_max_buffers((GstAppSink *)sink, 1);
    GstAppSinkCallbacks callbacks = {NULL, NULL, (GstFlowReturn(*)(GstAppSink *, gpointer))frame_recorded_callback};

    auto socketData = CustomData{rtp_stream = rtp_stream};
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &callbacks, &socketData, NULL);

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
