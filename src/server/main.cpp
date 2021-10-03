#include <iostream>
#include <lib.hh>

const int bpp = 12;
int screen_w = 1920, screen_h = 1000;
const int pixel_w = 1920, pixel_h = 1080;
unsigned char buffer[pixel_w * pixel_h * bpp / 8];
size_t TOTAL_PAYLOAD_LEN = pixel_w * pixel_h * bpp / 8;
size_t MAX_PAYLOAD_LEN = 1024;
size_t PAYLOAD_LEN = TOTAL_PAYLOAD_LEN / MAX_PAYLOAD_LEN;

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

    uvgrtp::context ctx;
    printf("Initializing the server\n");
    uvgrtp::session *session = ctx.create_session(hostname.c_str());
    printf("Initializing the server\n");
    uvgrtp::media_stream *rtp_stream = session->create_stream(receive_port, send_port, RTP_FORMAT_GENERIC, RCE_FRAGMENT_GENERIC);

    FILE *fp = NULL;
    fp = fopen("C:/Storage/Coding/c++/SdlVideo/x64/Release/output.yuv", "rb+");

    if (fp == NULL)
    {
        printf("cannot open this file\n");
        return -1;
    }

    std::string message("Hello, world!");
    for (;;)
    {
        if (fread(buffer, 1, pixel_w * pixel_h * bpp / 8, fp) != pixel_w * pixel_h * bpp / 8)
        {
            // Loop
            fseek(fp, 0, SEEK_SET);
            fread(buffer, 1, pixel_w * pixel_h * bpp / 8, fp);
        }
        printf("Sending frame to the client\n");
        // Starting sending the chunked frame header
        uint8_t data = NETWORK_PACKET::START_FRAME;
        rtp_stream->push_frame((uint8_t *)(&data), 1, RTP_NO_FLAGS);

        // Send chunks of the frame
        int sent = 0;
        while (TOTAL_PAYLOAD_LEN - sent > MAX_PAYLOAD_LEN)
        {
            rtp_stream->push_frame((uint8_t *)(buffer + sent), MAX_PAYLOAD_LEN, RTP_NO_FLAGS);
            sent += MAX_PAYLOAD_LEN;
        }
        rtp_stream->push_frame((uint8_t *)(buffer + sent), TOTAL_PAYLOAD_LEN - sent, RTP_NO_FLAGS);

        // Send chunks of the frame footer
        data = NETWORK_PACKET::END_FRAME;
        rtp_stream->push_frame((uint8_t *)(&data), 1, RTP_NO_FLAGS);
    }

    ctx.destroy_session(session);
    return 0;
}