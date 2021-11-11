#include <gst/gst.h>
#include <glib.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <stdio.h>
#include <iostream>
#include <lib.hh>
#include <string.h>
#include <vector>
#include <string.h>
#include <assert.h>

#include <SDL_events.h>

#include "pac_network.h"
#include "config.h"
#include "input_emulator.h"

Config configuration = Config("server.conf");

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

    ////////////
    for (int i = 0; i < 20; i += 20)
    {
        for (int j = 0; j < 20; j += 20)
        {
            SDL_Event fake_event;
            fake_event.type = SDL_MOUSEMOTION;
            fake_event.motion.x = i;
            fake_event.motion.y = j;
            handle_sdl_event(fake_event);
            Sleep(100);
        }
    }
    return 0;

    ////////////
    std::string hostname(configuration["hostname"]);
    auto receive_port = stoi(configuration["receive_port"]);
    auto send_port = stoi(configuration["send_port"]);

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
    auto pipeline_args = configuration["gstreamer_windows"].c_str();
#else
    auto pipeline_args = configuration["gstreamer_linux"].c_str();
#endif
#else
    auto pipeline_args = configuration["gstreamer_splashscreen"].c_str();
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
