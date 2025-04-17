//
// Created by user on 09.09.2021.
//

#if defined(ARA_USE_LIBRTMP) && (_WIN32)

#pragma once

#include <Network/rtmp/RtmpUtils.h>
#include <Network/rtmp/librtmp/log.h>
#include <Network/rtmp/librtmp/rtmp.h>

#include "Log.h"
#include "util_common.h"

namespace ara {

class RTMPSender {
public:
    RTMPSender() = default;

    RTMPSender(std::string &&url, bool live) { connect(std::move(url), live); }

    void connect(std::string &&url, bool live);
    void init_send();
    void send_thread();
    void stop();
    bool send_additional_meta_data();
    bool send_meta_data();
    bool send_headers(rtmp_stream &stream);
    bool send_audio_header(rtmp_stream &stream, size_t idx, bool *next);
    bool send_video_header(rtmp_stream &stream);

    std::vector<uint8_t> build_flv_meta_data();

    std::vector<uint8_t> flv_meta_data(bool write_header);

    std::vector<uint8_t> flv_build_additional_meta_data();

    std::vector<uint8_t> flv_additional_meta_data();

    std::vector<uint8_t> flv_build_additional_audio(encoder_packet &packet, bool is_header, size_t index);

    std::vector<uint8_t> flv_additional_audio(int32_t dts_offset, encoder_packet &packet, bool is_header, size_t index);

    std::vector<uint8_t> flv_additional_packet_mux(encoder_packet &packet, int32_t dts_offset, bool is_header,
                                                   size_t index);

    void flv_video(int32_t dts_offset, encoder_packet &packet, bool is_header);
    void flv_audio(int32_t dts_offset, encoder_packet &packet, bool is_header);
    void flv_packet_mux(encoder_packet &packet, int32_t dts_offset, bool is_header);
    int  send_packet(rtmp_stream &stream, encoder_packet &packet, bool is_header, size_t idx);

    const uint8_t *ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end);

    const uint8_t *avc_find_startcode(const uint8_t *p, const uint8_t *end);

    void parse_avc_header(encoder_packet &packet);

    std::vector<uint8_t> extract_avc_headers(const uint8_t *packet, size_t size, obs_encoder_type tp);

    bool avc_keyframe(const uint8_t *data, size_t size);
    bool discard_recv_data(rtmp_stream &stream, size_t size);
    void push_packet(int streamIdx, int64_t pts, int64_t dts, uint8_t *data, size_t size, obs_encoder_type tp);

    // extra data form ffmpeg seems something completely different, then extra
    // data which obs uses...
    void setVideoEncExtraData(uint8_t *data, int size) {
        m_vEncExtraData     = data;
        m_vEncExtraDataSize = size;
    }

    void setAudioEncExtraData(uint8_t *data, int size) {
        m_aEncExtraData     = data;
        m_aEncExtraDataSize = size;
    }

    void setHaveAudio(bool val) { m_haveAudio = val; }
    void setHaveVideo(bool val) { m_haveVideo = val; }

    void setVideoSize(uint32_t width, uint32_t height) {
        m_vidWidth  = width;
        m_vidHeight = height;
    }

    void    setFrameRate(uint32_t fps) { m_vidFrameRate = fps; }
    void    setVidBitrate(uint32_t br) { m_vidBitRate = br; }
    void    setAudBitrate(uint32_t br) { m_audBitRate = br; }
    void    setSampleRate(uint32_t sr) { m_sampleRate = sr; }
    void    setNrChannels(uint32_t nc) { m_audioNrChans = nc; }
    int32_t get_ms_time(encoder_packet &packet, int64_t val) { return (int32_t)(val * 1000 / packet.timebase_den); }
    RTMP   *getRtmpHnd() { return m_rtmp; }
    bool    isConnected() { return m_connected; }
    rtmp_stream &getStream() { return m_stream; }

#ifdef _WIN32
    void RTMPSender::win32_log_interface_type(RTMP *rtmp);
#endif

    bool has_start_code(const uint8_t *data);

    void get_sps_pps(const uint8_t *data, size_t size, const uint8_t **sps, size_t *sps_size, const uint8_t **pps,
                     size_t *pps_size);

    bool              first_packet = true;
    std::future<bool> m_connectFut;

private:
    RTMP       *m_rtmp = nullptr;
    rtmp_stream m_stream;

    std::string m_rtmp_url     = {0};
    std::string m_appName      = {0};
    std::string m_user         = {0};
    std::string m_password     = {0};
    std::string m_streamName   = {0};
    std::string m_encoder_name = {0};

    std::atomic<bool> m_sendLoopRunning;
    std::thread       m_sendThread;

    uint8_t *m_vEncExtraData     = nullptr;
    int      m_vEncExtraDataSize = 0;
    uint8_t *m_aEncExtraData     = nullptr;
    int      m_aEncExtraDataSize = 0;

    bool m_low_latency_mode = true;
    bool m_haveAudio        = false;
    bool m_haveVideo        = false;
    bool m_connected        = false;

    uint32_t m_vidWidth     = 0;
    uint32_t m_vidHeight    = 0;
    uint32_t m_vidFrameRate = 0;
    uint32_t m_vidBitRate   = 0;
    uint32_t m_audBitRate   = 0;
    uint32_t m_sampleRate   = 0;
    uint32_t m_audioNrChans = 0;

    std::vector<uint8_t> m_sei;
    std::vector<uint8_t> m_videoHeader;
};

}  // namespace ara

#endif