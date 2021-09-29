#include <iostream>
#include <lib.hh>

int main()
{
    std::cout << "I'm the server" << std::endl;

    uvgrtp::context ctx;
    uvgrtp::session *sess = ctx.create_session("127.0.0.1");

    uvgrtp::media_stream *strm = sess->create_stream(8888, 8888, RTP_FORMAT_GENERIC, RTP_NO_FLAGS);

    char *message = (char *)"Hello, world!";
    size_t msg_len = strlen(message) + 1;

    for (;;)
    {
        strm->push_frame((uint8_t *)message, msg_len, RTP_NO_FLAGS);
        auto frame = strm->pull_frame();
        fprintf(stderr, "Message: '%s'\n", frame->payload);
        uvgrtp::frame::dealloc_frame(frame);
    }
    return 0;
}