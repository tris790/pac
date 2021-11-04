#include <iostream>
#include <lib.hh>
#include <string.h>
#include <vector>
#include "config.h"

Config conf = Config("D:/Data_mick/Universite/projet/pac/src/server/.env");
const int bpp = stoi(conf.conf_dict["bpp"]);
int screen_w = stoi(conf.conf_dict["screen_w"]), screen_h = stoi(conf.conf_dict["screen_h"]);
const int pixel_w = stoi(conf.conf_dict["pixel_w"]), pixel_h = stoi(conf.conf_dict["pixel_h"]);
size_t TOTAL_FRAME_LEN = pixel_w * pixel_h * bpp / 8;
size_t MAX_DATAFRAME_LEN = stoi(conf.conf_dict["MAX_DATAFRAME_LEN"]); // 5120;

enum NETWORK_PACKET
{
  START_FRAME,
  END_FRAME
};

int main()
{
  
  std::map<std::string, std::string>::iterator it = conf.conf_dict.begin();
  // imprime les valeurs de la config en console. probablement a retirer
  for(std::pair<std::string, std::string> element : conf.conf_dict)
  {
   printf( element.first.append(":").append(element.second.append("\n")).c_str());
  } 
  std::string hostname(conf.conf_dict["hostname"]);
  auto receive_port = stoi(conf.conf_dict["receive_port"]);
  auto send_port = stoi(conf.conf_dict["send_port"]);
  auto buffer = new unsigned char[TOTAL_FRAME_LEN];

  uvgrtp::context ctx;
  printf("Initializing the server\n");
  uvgrtp::session *session = ctx.create_session(hostname.c_str());
  printf("Initializing the server\n");
  // checkout RCC_UDP_SND_BUF_SIZE and RCC_UDP_RCV_BUF_SIZE
  // https://github.com/ultravideo/uvgRTP/issues/76
  uvgrtp::media_stream *rtp_stream = session->create_stream(
      receive_port, send_port, RTP_FORMAT_GENERIC, RCE_FRAGMENT_GENERIC);

  std::string path = conf.conf_dict["static_video_path"];
  FILE *fp = NULL;
  fopen_s(&fp,path.c_str(), "rb+");

  if (fp == NULL)
  {
    printf("cannot open this file %s\n", path.c_str());
    return -1;
  }

  while (true)
  {
    // Read frame from file
    if (fread(buffer, 1, TOTAL_FRAME_LEN, fp) != TOTAL_FRAME_LEN)
    {
      // Loop
      fseek(fp, 0, SEEK_SET);
      fread(buffer, 1, TOTAL_FRAME_LEN, fp);
      printf("Start sending video to client");
    }

    // Send frame header
    uint8_t data = NETWORK_PACKET::START_FRAME;
    rtp_stream->push_frame((uint8_t *)(&data), 1, RTP_NO_FLAGS);

    // Send chunks of the frame
    size_t sent = 0;
    while (TOTAL_FRAME_LEN - sent > MAX_DATAFRAME_LEN)
    {
      rtp_stream->push_frame((uint8_t *)(buffer + sent), MAX_DATAFRAME_LEN,
                             RTP_NO_FLAGS);
      sent += MAX_DATAFRAME_LEN;
    }
    rtp_stream->push_frame((uint8_t *)(buffer + sent), TOTAL_FRAME_LEN - sent,
                           RTP_NO_FLAGS);

    // Send frame ending
    data = NETWORK_PACKET::END_FRAME;
    rtp_stream->push_frame((uint8_t *)(&data), 1, RTP_NO_FLAGS);
    std::this_thread::sleep_for(std::chrono::milliseconds(13));
  }

  ctx.destroy_session(session);
  return 0;
}
