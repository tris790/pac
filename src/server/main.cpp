#include <iostream>
#include <lib.hh>
#include <vector>

const int bpp = 12;
int screen_w = 1920, screen_h = 1000;
const int pixel_w = 1920, pixel_h = 1080;
size_t TOTAL_FRAME_LEN = pixel_w * pixel_h * bpp / 8;
size_t MAX_DATAFRAME_LEN = 1024; //5120;

enum NETWORK_PACKET
{
    START_FRAME,
    END_FRAME
};

int main()
{
    std::string hostname("127.0.0.1");
    auto receive_port = 8888;
    auto send_port = 8889;
    auto buffer = new unsigned char[TOTAL_FRAME_LEN];

    uvgrtp::context ctx;
    printf("Initializing the server\n");
    uvgrtp::session *session = ctx.create_session(hostname.c_str());
    printf("Initializing the server\n");
    // checkout RCC_UDP_SND_BUF_SIZE and RCC_UDP_RCV_BUF_SIZE
    // https://github.com/ultravideo/uvgRTP/issues/76
    uvgrtp::media_stream *rtp_stream = session->create_stream(receive_port, send_port, RTP_FORMAT_GENERIC, RCE_FRAGMENT_GENERIC);

    FILE *fp = NULL;
    fp = fopen("C:/Storage/Coding/c++/SdlVideo/x64/Release/output.yuv", "rb+");

    if (fp == NULL)
    {
        printf("cannot open this file\n");
        return -1;
    }

    for (;;)
    {
        // Read frame from file
        if (fread(buffer, 1, TOTAL_FRAME_LEN, fp) != TOTAL_FRAME_LEN)
        {
            // Loop
            fseek(fp, 0, SEEK_SET);
            fread(buffer, 1, TOTAL_FRAME_LEN, fp);
        }

        printf("Sending frame to the client\n");
        // Send frame header
        uint8_t data = NETWORK_PACKET::START_FRAME;
        rtp_stream->push_frame((uint8_t *)(&data), 1, RTP_NO_FLAGS);

        // Send chunks of the frame
        int sent = 0;
        while (TOTAL_FRAME_LEN - sent > MAX_DATAFRAME_LEN)
        {
            rtp_stream->push_frame((uint8_t *)(buffer + sent), MAX_DATAFRAME_LEN, RTP_NO_FLAGS);
            sent += MAX_DATAFRAME_LEN;
        }
        rtp_stream->push_frame((uint8_t *)(buffer + sent), TOTAL_FRAME_LEN - sent, RTP_NO_FLAGS);

        // Send frame ending
        data = NETWORK_PACKET::END_FRAME;
        rtp_stream->push_frame((uint8_t *)(&data), 1, RTP_NO_FLAGS);
        std::this_thread::sleep_for(std::chrono::milliseconds(13));
    }

    ctx.destroy_session(session);
    return 0;
}