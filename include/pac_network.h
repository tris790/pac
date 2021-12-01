#pragma once

const size_t MAX_DATAFRAME_LEN = 1024; // 1518 default max linux;

enum NETWORK_PACKET_TYPE
{
    REMOTE_INPUT
};

typedef struct _NetworkPacket
{
    NETWORK_PACKET_TYPE packet_type;
    char data[MAX_DATAFRAME_LEN - sizeof(packet_type)];
} NetworkPacket;