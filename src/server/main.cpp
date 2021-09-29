#include <iostream>
#include <lib.hh>

int main()
{
    std::string hostname("127.0.0.1");
    auto receive_port = 8888;
    auto send_port = 8889;

    uvgrtp::context ctx;
    printf("Initializing the server\n");
    uvgrtp::session *session = ctx.create_session(hostname.c_str());
    printf("Initializing the server\n");
    uvgrtp::media_stream *rtp_stream = session->create_stream(receive_port, send_port, RTP_FORMAT_GENERIC, RTP_NO_FLAGS);

    std::string message("Hello, world!");
    for (;;)
    {
        printf("Sending '%s' to the client\n", message.c_str());
        rtp_stream->push_frame((uint8_t *)(message.c_str()), message.length() + 1, RTP_NO_FLAGS);
    }

    ctx.destroy_session(session);
    return 0;
}