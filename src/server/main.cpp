#include <gst/gst.h>
#include <glib.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <string.h>
#include <assert.h>

#include "pac_network.h"
#include "config.h"

Config configuration = Config("server.conf");

#define GSTREAMER_CAPTURE 1

int main(int argc, char *argv[])
{
    printf("Initializing the server\n");

    std::string hostname(configuration["hostname"]);

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

    return 0;
}
