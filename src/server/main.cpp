#include <iostream>
#include <lib.hh>

int main()
{
    std::cout << "I'm the server" << std::endl;

    uvgrtp::context ctx;
    printf("Server1\n");
    uvgrtp::session *sess = ctx.create_session("127.0.0.1");
    printf("Server2\n");

    uvgrtp::media_stream *strm = sess->create_stream(8888, 8889, RTP_FORMAT_GENERIC, RTP_NO_FLAGS);
    printf("Server3\n");

    char *message = (char *)"Hello, world!";
    size_t msg_len = strlen(message) + 1;
    printf("Server4\n");

    for (;;)
    {
        printf("Server5\n");
        strm->push_frame((uint8_t *)message, msg_len, RTP_NO_FLAGS);
        printf("Server6\n");
        // auto frame = strm->pull_frame();
        // fprintf(stderr, "Message: '%s'\n", frame->payload);
        // uvgrtp::frame::dealloc_frame(frame);
    }
    return 0;
}