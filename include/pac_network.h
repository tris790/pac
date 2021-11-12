#pragma once

const size_t MAX_DATAFRAME_LEN = 1024; // 1518 default max linux;

enum NETWORK_PACKET_TYPE
{
    START_FRAME,
    END_FRAME,
    VIDEO,
    FLUSH_SCREEN
};

typedef struct _NetworkPacket
{
    NETWORK_PACKET_TYPE packet_type;
    int buffer_offset;
    char data[MAX_DATAFRAME_LEN - sizeof(packet_type) - sizeof(buffer_offset)];
} NetworkPacket;