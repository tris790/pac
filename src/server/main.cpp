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
#include <unistd.h>
#include <SDL.h>
#include <SDL_events.h>

#include "pac_network.h"
#include "config.h"
#include "input_emulator.h"
#include "logger.h"

Config configuration = Config("server.conf");

#define GSTREAMER_CAPTURE 1

int thread_exit = 0;

int network_thread_fn(void *rtp_stream_arg)
{
    auto rtp_stream = (uvgrtp::media_stream *)rtp_stream_arg;

    while (!thread_exit)
    {
        uvgrtp::frame::rtp_frame *input_network_frame = rtp_stream->pull_frame(5);

        if (input_network_frame)
        {
            auto input_packet = (NetworkPacket *)input_network_frame->payload;

            // If received input
            if (input_packet->packet_type == NETWORK_PACKET_TYPE::INPUT)
            {
                logger.debug("Server input received");

                auto packet = input_packet->data;

                // Call trist code input
            }
        }

        uvgrtp::frame::dealloc_frame(input_network_frame);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    logger.info("Initializing the server");

    ////////////
    // for (int i = 0; i < 20; i++)
    // {
    //     for (int j = 0; j < 20; j++)
    //     {
    //         SDL_Event fake_event;
    //         fake_event.type = SDL_KEYDOWN;
    //         fake_event.button.button = SDLK_a;
    //         handle_sdl_event(fake_event);
    //         sleep(1000);
    //     }
    // }
    // return 0;
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

    SDL_Thread *network_thread = SDL_CreateThread(network_thread_fn, NULL, rtp_stream);

    uvgrtp::frame::rtp_frame *data = rtp_stream->pull_frame();

    if (data)
    {
        //auto packet = data->payload;
        //auto packet_size = data->payload_len;
        //logger.debug("Client size: %lld", packet_size);
    }

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

    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    logger.debug("[Now] Pipeline started");

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
        logger.error("GST_MESSAGE_ERROR - %s", err->message);
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
