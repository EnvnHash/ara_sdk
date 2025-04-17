//
// Created by user on 09.09.2021.
//

#ifdef ARA_USE_LIBRTMP

#include "Network/rtmp/RTMPSender.h"
#include "Network/rtmp/rtmp-helpers.h"
#include "string_utils.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <iphlpapi.h>
#include <ipmib.h>
#endif

using namespace std;

namespace ara {

void RTMPSender::connect(std::string &&url, bool live) {
    m_rtmp_url = url;

    //    m_connectFut = async(launch::async, [this]{
    // split url to separate app_name and stream name
    auto urlSep = split(m_rtmp_url, "/");
    if (urlSep.size() < 5) {
        LOGE << "error url to short, possibly missing stream name";
        return;
    }

    m_appName    = urlSep[3];
    m_streamName = urlSep[4];
    m_rtmp_url   = m_rtmp_url.substr(0, m_rtmp_url.size() - m_streamName.size() - 1);

    // add this if doing live streaming: ... doesn't work
    // if (live) m_rtmp_url.append(" live=1");

    m_rtmp = RTMP_Alloc();
    if (!m_rtmp) {
        LOGE << "Unable to create rtmp object\n";
        return;
    }

    RTMP_Init(m_rtmp);
    RTMP_LogSetLevel(RTMP_LOGDEBUG2);
    // RTMP_LogSetLevel(RTMP_LOGINFO);
    RTMP_LogSetOutput(stderr);

    char *rtmpUrlChar = &m_rtmp_url[0];
    RTMP_SetupURL(m_rtmp, rtmpUrlChar);  // librtmp is a mess

    RTMP_EnableWrite(m_rtmp);

    m_rtmp->Link.pubUser.av_val   = (char *)m_user.c_str();
    m_rtmp->Link.pubUser.av_len   = (int)m_user.size();
    m_rtmp->Link.pubPasswd.av_val = (char *)m_password.c_str();
    m_rtmp->Link.pubPasswd.av_len = (int)m_password.size();

    m_encoder_name               = "FMLE/3.0 (compatible; FMSc/1.0)";
    m_rtmp->Link.flashVer.av_val = (char *)m_encoder_name.c_str();
    m_rtmp->Link.flashVer.av_len = (int)m_encoder_name.size();

    m_rtmp->Link.swfUrl = m_rtmp->Link.tcUrl;
    memset(&m_rtmp->m_bindIP, 0, sizeof(m_rtmp->m_bindIP));

    RTMP_AddStream(m_rtmp, m_streamName.c_str());

    m_rtmp->m_outChunkSize       = 4096;
    m_rtmp->m_bSendChunkSizeInfo = true;
    m_rtmp->m_bUseNagle          = true;

#ifdef _WIN32
    win32_log_interface_type(m_rtmp);
#endif

    // connect to server
    if (!RTMP_Connect(m_rtmp, nullptr)) {
        LOGE << "Unable to connect to server";
        RTMP_Free(m_rtmp);
        return;
    }

    // connect to stream (this will be the stream specified in the RTMP URL)
    if (!RTMP_ConnectStream(m_rtmp, 0)) {
        LOGE << "Unable to connect to stream\n";
        RTMP_Free(m_rtmp);
        return;
    }

    LOG << "RTMPSender connected to " << m_rtmp_url;

    m_connected = true;
    init_send();
}

void RTMPSender::init_send() {
    if (!m_connected) return;

    if (!send_meta_data()) LOGE << "Disconnected while attempting to send metadata";

    if (m_haveAudio)
        if (!send_additional_meta_data()) LOGE << "Disconnected while attempting to send metadata";

    m_sendLoopRunning = true;
    m_sendThread      = thread(&RTMPSender::send_thread, this);
    m_sendThread.detach();
}

void RTMPSender::send_thread() {
    while (m_sendLoopRunning) {
        dbr_frame dbr_frame;

        if (m_stream.stopping && m_stream.stop_ts == 0) break;

        // if we dont't have packets, wait a bit and continue
        if (!m_stream.writePos) {
            std::this_thread::sleep_for(100us);
            continue;
        }

        if (m_stream.stopping) {
            //            if (can_shutdown_stream(m_stream, &packet))
            //            {
            break;
            //            }
        }

        if (!m_stream.sent_headers) {
            LOG << "trying to sent headers";
            while (!send_headers(m_stream)) {
                this_thread::sleep_for(chrono::milliseconds(10));
                // m_stream.disconnected = true;
                // break;
            }
            LOG << " headers sent!!!!";
        }

        // NOTE: at this point the receiver should have all necessary
        // information about the stream

        if (m_stream.dbr_enabled) {
            dbr_frame.send_beg = os_gettime_ns();
            dbr_frame.size     = m_stream.packets[m_stream.readPos].data.size();
        }

        if (send_packet(m_stream, m_stream.packets[m_stream.readPos], false,
                        m_stream.packets[m_stream.readPos].track_idx) < 0) {
            m_stream.disconnected = true;
            break;
        }

        m_stream.readPos = ++m_stream.readPos % m_stream.packets.size();

        if (m_stream.dbr_enabled) {
            dbr_frame.send_end = os_gettime_ns();

            m_stream.dbr_mutex.lock();
            // dbr_add_frame(m_stream, &dbr_frame);
            m_stream.dbr_mutex.unlock();
        }
    }
    /*
        bool encode_error = os_atomic_load_bool(&stream->encode_error);

        if (disconnected(stream))
        {
            info("Disconnected from %s", stream->path.array);
        } else if (encode_error) {
            LOG << "Encoder error, disconnecting";
        } else {
            LOG << "User stopped the stream";
        }

        if (stream->new_socket_loop) {
            os_event_signal(stream->send_thread_signaled_exit);
            os_event_signal(stream->buffer_has_data_event);
            pthread_join(stream->socket_thread, NULL);
            stream->socket_thread_active = false;
            stream->rtmp.m_bCustomSend = false;
        }

        set_output_error(stream);
        RTMP_Close(&stream->rtmp);

        if (!stopping(stream))
        {
            pthread_detach(stream->send_thread);
            obs_output_signal_stop(stream->output, OBS_OUTPUT_DISCONNECTED);
        } else if (encode_error) {
            obs_output_signal_stop(stream->output, OBS_OUTPUT_ENCODE_ERROR);
        } else {
            obs_output_end_data_capture(stream->output);
        }

        free_packets(stream);
        os_event_reset(stream->stop_event);
        os_atomic_set_bool(&stream->active, false);
        stream->sent_headers = false;

        // reset bitrate on stop
        if (stream->dbr_enabled)
        {
            if (stream->dbr_cur_bitrate != stream->dbr_orig_bitrate)
            {
                stream->dbr_cur_bitrate = stream->dbr_orig_bitrate;
                dbr_set_bitrate(stream);
            }
        }*/
}

bool RTMPSender::send_additional_meta_data() {
    auto packet = flv_additional_meta_data();
    return RTMP_Write(m_rtmp, (char *)&packet[0], (int)packet.size(), 0) >= 0;
}

bool RTMPSender::send_meta_data() {
    auto packet = flv_meta_data(false);
    return RTMP_Write(m_rtmp, (char *)&packet[0], (int)packet.size(), 0) >= 0;
}

bool RTMPSender::send_headers(rtmp_stream &stream) {
    stream.sent_headers = true;
    size_t i            = 0;
    bool   next         = true;

    // if (m_haveAudio)
    //     if (!send_audio_header(stream, i++, &next))
    //         return false;

    if (m_haveVideo)
        if (!send_video_header(stream)) return false;

    //  if (m_haveAudio)
    //      while (next)
    //         if (!send_audio_header(stream, i++, &next))
    //             return false;

    return true;
}

bool RTMPSender::send_audio_header(rtmp_stream &stream, size_t idx, bool *next) {
    encoder_packet packet;
    packet.type         = OBS_ENCODER_AUDIO;
    packet.timebase_den = 1;

    bool ret = false;
    if (m_aEncExtraData && m_aEncExtraDataSize) {
        if (packet.data.size() != m_aEncExtraDataSize) packet.data.resize(m_aEncExtraDataSize);

        memcpy(&packet.data[0], m_aEncExtraData, m_aEncExtraDataSize);

        ret = send_packet(stream, packet, true, 0) >= 0;
    }

    *next = false;  // signal send success
    return ret;
}

bool RTMPSender::send_video_header(rtmp_stream &stream) {
    encoder_packet packet;
    packet.type         = OBS_ENCODER_VIDEO;
    packet.timebase_den = 1;
    packet.keyframe     = true;

    if (m_videoHeader.empty()) return false;

    parse_avc_header(packet);
    LOG << "packet.size: ", packet.data.size();

    /*
        FILE *file = fopen("C:\\Users\\user\\Desktop\\video_header_dump.txt",
       "r"); packet.data.resize(53); int results = fread((char*)
       &packet.data[0], 1, 53, file); fclose(file);
    */

    /*
        bool ret = false;
        if (m_vEncExtraData && m_vEncExtraDataSize)
        {
            if (packet.data.size() != m_vEncExtraDataSize)
                packet.data.resize(m_vEncExtraDataSize);

            memcpy(&packet.data[0], m_vEncExtraData, m_vEncExtraDataSize);

           // packet.size = obs_parse_avc_header(&packet.data, m_vEncExtraData,
       m_vEncExtraDataSize); FILE *file =
       fopen("C:\\Users\\user\\Desktop\\ara_video_header_dump.txt", "w"); int
       results = fwrite((char*) &packet.data[0], 1, (int)packet.data.size(),
       file); if (results == EOF) { printf("write failed \n");
                // Failed to write do error code here.
            }
            fclose(file);

            ret = send_packet(stream, packet, true, 0) >= 0;
        }
    */

    bool ret = send_packet(stream, packet, true, 0) >= 0;

    return ret;
}

std::vector<uint8_t> RTMPSender::build_flv_meta_data() {
    char        buf[4096];
    char       *enc = buf;
    char       *end = enc + sizeof(buf);
    std::string encoder_name;

    enc_str(&enc, end, "@setDataFrame");
    enc_str(&enc, end, "onMetaData");

    *enc++ = AMF_ECMA_ARRAY;
    enc    = AMF_EncodeInt32(enc, end, 20);

    enc_num_val(&enc, end, "duration", 0.0);
    enc_num_val(&enc, end, "fileSize", 0.0);

    enc_num_val(&enc, end, "width", (double)m_vidWidth);
    enc_num_val(&enc, end, "height", (double)m_vidHeight);

    enc_num_val(&enc, end, "videocodecid", VIDEODATA_AVCVIDEOPACKET);
    enc_num_val(&enc, end, "videodatarate", (double)m_vidBitRate);
    enc_num_val(&enc, end, "framerate", (double)m_vidFrameRate);

    enc_num_val(&enc, end, "audiocodecid", AUDIODATA_AAC);
    enc_num_val(&enc, end, "audiodatarate", (double)m_audBitRate);
    enc_num_val(&enc, end, "audiosamplerate", (double)m_sampleRate);
    enc_num_val(&enc, end, "audiosamplesize", 16.0);
    enc_num_val(&enc, end, "audiochannels", (double)m_audioNrChans);

    enc_bool_val(&enc, end, "stereo", m_audioNrChans == 2);
    enc_bool_val(&enc, end, "2.1", m_audioNrChans == 3);
    enc_bool_val(&enc, end, "3.1", m_audioNrChans == 4);
    enc_bool_val(&enc, end, "4.0", m_audioNrChans == 4);
    enc_bool_val(&enc, end, "4.1", m_audioNrChans == 5);
    enc_bool_val(&enc, end, "5.1", m_audioNrChans == 6);
    enc_bool_val(&enc, end, "7.1", m_audioNrChans == 8);

    encoder_name = "UnVue (version 0.1)";
    enc_str_val(&enc, end, "decoder", encoder_name.c_str());

    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    size_t               size = enc - buf;
    std::vector<uint8_t> outBuf(enc - buf);
    memcpy(&outBuf[0], buf, size);

    return std::move(outBuf);
}

std::vector<uint8_t> RTMPSender::flv_meta_data(bool write_header) {
    std::vector<uint8_t> packet;

    if (write_header) {
        packet.push_back('F');  // signature byte
        packet.push_back('L');  // signature byte
        packet.push_back('V');  // signature byte
        packet.push_back(1);    // file version
        packet.push_back(5);    // audio and or video
        push_wb32(packet,
                  9);  // data offset from start of file to start of body (that
                       // is size of header), usually 9 for FLV versino 1
        push_wb32(packet,
                  0);  // data offset from start of file to start of body (that
                       // is size of header), usually 9 for FLV versino 1
    }

    size_t start_pos = packet.size();

    packet.push_back(RTMP_PACKET_TYPE_INFO);

    auto metaData = build_flv_meta_data();
    push_wb24(packet, (uint32_t)metaData.size());
    push_wb32(packet, 0);
    push_wb24(packet, 0);

    packet.insert(packet.end(), metaData.begin(), metaData.end());

    push_wb32(packet, (uint32_t)packet.size() - (uint32_t)start_pos - 1);

    return std::move(packet);
}

std::vector<uint8_t> RTMPSender::flv_build_additional_meta_data() {
    std::vector<uint8_t> packet;

    packet.emplace_back(AMF_STRING);
    push_str(packet, "@setDataFrame");

    packet.emplace_back(AMF_STRING);
    push_str(packet, "onExpectAdditionalMedia");

    packet.emplace_back(AMF_OBJECT);
    {
        push_str(packet, "processingIntents");

        packet.emplace_back(AMF_STRICT_ARRAY);
        push_wb32(packet, 1);
        {
            packet.emplace_back(AMF_STRING);
            push_str(packet, "ArchiveProgramNarrationAudio");
        }

        /* ---- */

        push_str(packet, "additionalMedia");

        packet.emplace_back(AMF_OBJECT);
        {
            push_str(packet, "stream0");

            packet.emplace_back(AMF_OBJECT);
            {
                push_str(packet, "type");

                packet.emplace_back(AMF_NUMBER);
                double d_val = RTMP_PACKET_TYPE_AUDIO;
                push_wb64(packet, *(uint64_t *)&d_val);

                /* ---- */

                push_str(packet, "mediaLabels");

                packet.emplace_back(AMF_OBJECT);
                {
                    push_str(packet, "contentType");

                    packet.emplace_back(AMF_STRING);
                    push_str(packet, "PNAR");
                }
                push_wb24(packet, AMF_OBJECT_END);
            }
            push_wb24(packet, AMF_OBJECT_END);
        }
        push_wb24(packet, AMF_OBJECT_END);

        /* ---- */

        push_str(packet, "defaultMedia");

        packet.emplace_back(AMF_OBJECT);
        {
            push_str(packet, "audio");

            packet.emplace_back(AMF_OBJECT);
            {
                push_str(packet, "mediaLabels");

                packet.emplace_back(AMF_OBJECT);
                {
                    push_str(packet, "contentType");

                    packet.emplace_back(AMF_STRING);
                    push_str(packet, "PRM");
                }
                push_wb24(packet, AMF_OBJECT_END);
            }
            push_wb24(packet, AMF_OBJECT_END);
        }
        push_wb24(packet, AMF_OBJECT_END);
    }
    push_wb24(packet, AMF_OBJECT_END);

    return std::move(packet);
}

std::vector<uint8_t> RTMPSender::flv_build_additional_audio(encoder_packet &packet, bool is_header, size_t index) {
    UNUSED_PARAMETER(index);
    std::vector<uint8_t> data;

    data.emplace_back(AMF_STRING);
    push_str(data, "additionalMedia");

    data.emplace_back(AMF_OBJECT);
    {
        push_str(data, "id");

        data.emplace_back(AMF_STRING);
        push_str(data, "stream0");

        /* ----- */

        push_str(data, "media");

        data.emplace_back(AMF_AVMPLUS);
        data.emplace_back(AMF3_BYTE_ARRAY);
        s_u29b_value(data, (uint32_t)packet.data.size() + 2);
        data.emplace_back(0xaf);
        data.emplace_back(is_header ? 0 : 1);
        data.insert(data.end(), packet.data.begin(), packet.data.end());
    }
    push_wb24(data, AMF_OBJECT_END);
    return std::move(data);
}

std::vector<uint8_t> RTMPSender::flv_additional_audio(int32_t dts_offset, encoder_packet &packet, bool is_header,
                                                      size_t index) {
    int32_t time_ms = get_ms_time(packet, packet.dts) - dts_offset;

    auto data = flv_build_additional_audio(packet, is_header, index);

    data.emplace_back(RTMP_PACKET_TYPE_INFO);  // 18

#ifdef DEBUG_TIMESTAMPS
    blog(LOG_DEBUG, "Audio2: %lu", time_ms);

    if (last_time > time_ms) blog(LOG_DEBUG, "Non-monotonic");

    last_time = time_ms;
#endif

    push_wb24(data, (uint32_t)data.size());
    push_wb24(data, time_ms);
    data.emplace_back((time_ms >> 24) & 0x7F);
    push_wb24(data, 0);
    push_wb32(data, (uint32_t)data.size() - 1);

    return std::move(data);
}

std::vector<uint8_t> RTMPSender::flv_additional_meta_data() {
    auto metaData = flv_build_additional_meta_data();

    std::vector<uint8_t> packet;

    packet.emplace_back(RTMP_PACKET_TYPE_INFO);  // 18

    push_wb24(packet, (uint32_t)metaData.size());
    push_wb32(packet, 0);
    push_wb24(packet, 0);

    packet.insert(packet.end(), metaData.begin(), metaData.end());

    push_wb32(packet, (uint32_t)packet.size() - 1);

    return std::move(packet);
}

std::vector<uint8_t> RTMPSender::flv_additional_packet_mux(encoder_packet &packet, int32_t dts_offset, bool is_header,
                                                           size_t index) {
    if (packet.type == OBS_ENCODER_VIDEO) {
        // currently unsupported
        LOGE << "who said you could output an additional video packet?";
        return std::move(std::vector<uint8_t>());
    } else {
        return std::move(flv_additional_audio(dts_offset, packet, is_header, index));
    }
}

void RTMPSender::flv_video(int32_t dts_offset, encoder_packet &packet, bool is_header) {
    int64_t offset  = packet.pts - packet.dts;
    int32_t time_ms = get_ms_time(packet, packet.dts) - dts_offset;

    vector<uint8_t> data;
    data.emplace_back(RTMP_PACKET_TYPE_VIDEO);

#ifdef DEBUG_TIMESTAMPS
    blog(LOG_DEBUG, "Video: %lu", time_ms);

    if (last_time > time_ms) blog(LOG_DEBUG, "Non-monotonic");

    last_time = time_ms;
#endif

    push_wb24(data, (uint32_t)packet.data.size() + 5);
    push_wb24(data, time_ms);
    data.emplace_back((time_ms >> 24) & 0x7F);
    push_wb24(data, 0);

    /* these are the 5 extra bytes mentioned above */
    data.emplace_back(packet.keyframe ? 0x17 : 0x27);
    data.emplace_back(is_header ? 0 : 1);
    push_wb24(data, get_ms_time(packet, offset));
    packet.data.insert(packet.data.begin(), data.begin(), data.end());

    /* write tag size (starting byte doesn't count) */
    push_wb32(packet.data, (uint32_t)packet.data.size() - 1);
}

void RTMPSender::flv_audio(int32_t dts_offset, encoder_packet &packet, bool is_header) {
    int32_t time_ms = get_ms_time(packet, packet.dts) - dts_offset;

    auto data = packet.data;
    data.emplace_back(RTMP_PACKET_TYPE_AUDIO);

#ifdef DEBUG_TIMESTAMPS
    blog(LOG_DEBUG, "Audio: %lu", time_ms);

    if (last_time > time_ms) blog(LOG_DEBUG, "Non-monotonic");

    last_time = time_ms;
#endif

    push_wb24(data, (uint32_t)packet.data.size() + 2);
    push_wb24(data, time_ms);
    data.emplace_back((time_ms >> 24) & 0x7F);
    push_wb24(data, 0);

    /* these are the two extra bytes mentioned above */
    data.emplace_back(0xaf);
    data.emplace_back(is_header ? 0 : 1);
    data.insert(data.end(), packet.data.begin(), packet.data.end());

    /* write tag size (starting byte doesn't count) */
    push_wb32(data, (uint32_t)data.size() - 1);
}

void RTMPSender::flv_packet_mux(encoder_packet &packet, int32_t dts_offset, bool is_header) {
    if (packet.type == OBS_ENCODER_VIDEO)
        flv_video(dts_offset, packet, is_header);
    else
        flv_audio(dts_offset, packet, is_header);
}

int RTMPSender::send_packet(rtmp_stream &stream, encoder_packet &packet, bool is_header, size_t idx) {
    unique_lock<mutex> l(packet.mutex);

    int recv_size = 0;
    int ret       = 0;

    assert(packet.track_idx < RTMP_MAX_STREAMS);

    if (!m_rtmp || packet.data.empty()) return -1;

    if (!stream.new_socket_loop) {
#ifdef _WIN32
        ret = ioctlsocket(m_rtmp->m_sb.sb_socket, FIONREAD, (u_long *)&recv_size);
#else
        ret = ioctl(m_rtmp->m_sb.sb_socket, FIONREAD, &recv_size);
#endif
        if (ret >= 0 && recv_size > 0) {
            if (!discard_recv_data(stream, (size_t)recv_size)) return -1;
        }
    }

    // is track_idx correct???
    if (idx > 0) {
        flv_additional_packet_mux(packet, is_header ? 0 : stream.start_dts_offset, is_header, idx);
    } else {
        flv_packet_mux(packet, is_header ? 0 : stream.start_dts_offset, is_header);
    }

#ifdef TEST_FRAMEDROPS
    droptest_cap_data_rate(stream, size);
#endif

    // ret = RTMP_Write(m_rtmp, (char *)data, (int)size, 0);
    ret = RTMP_Write(m_rtmp, (char *)&packet.data[0], (int)packet.data.size(), 0);

    stream.total_bytes_sent += packet.data.size();
    packet.data.clear();

    return ret;
}

/* NOTE: I noticed that FFmpeg does some unusual special handling of certain
 * scenarios that I was unaware of, so instead of just searching for {0, 0, 1}
 * we'll just use the code from FFmpeg - http://www.ffmpeg.org/ */
const uint8_t *RTMPSender::ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end) {
    const uint8_t *a = p + 4 - ((intptr_t)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1) return p;
    }

    for (end -= 3; p < end; p += 4) {
        uint32_t x = *(const uint32_t *)p;

        if ((x - 0x01010101) & (~x) & 0x80808080) {
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1) return p;
                if (p[2] == 0 && p[3] == 1) return p + 1;
            }

            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1) return p + 2;
                if (p[4] == 0 && p[5] == 1) return p + 3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1) return p;
    }

    return end + 3;
}

void RTMPSender::parse_avc_header(encoder_packet &packet) {
    const uint8_t *sps = nullptr, *pps = nullptr;
    size_t         sps_size = 0, pps_size = 0;

    if (m_videoHeader.size() <= 6) return;

    if (!has_start_code(&m_videoHeader[0])) {
        return;
    }

    get_sps_pps(&m_videoHeader[0], m_videoHeader.size(), &sps, &sps_size, &pps, &pps_size);
    if (!sps || !pps || sps_size < 4) return;

    vector<uint8_t> data;
    data.emplace_back(0x01);

    // s_write(&s, sps + 1, 3);
    data.resize(data.size() + 3);
    memcpy(&data[0], sps + 1, 3);

    data.emplace_back(0xff);
    data.emplace_back(0xe1);

    push_wb16(data, (uint16_t)sps_size);

    // s_write(&s, sps, sps_size);
    data.resize(data.size() + sps_size);
    memcpy(&data[0], sps, sps_size);

    data.emplace_back(0x01);
    push_wb16(data, (uint16_t)pps_size);

    // s_write(&s, pps, pps_size);
    data.resize(data.size() + pps_size);
    memcpy(&data[0], pps, pps_size);

    packet.data.clear();
    packet.data.insert(packet.data.begin(), data.begin(), data.end());
    //*header = output.bytes.array;
    // return output.bytes.num;
}

const uint8_t *RTMPSender::avc_find_startcode(const uint8_t *p, const uint8_t *end) {
    const uint8_t *out = ff_avc_find_startcode_internal(p, end);
    if (p < out && out < end && !out[-1]) out--;
    return out;
}

std::vector<uint8_t> RTMPSender::extract_avc_headers(const uint8_t *packet, size_t size, obs_encoder_type tp) {
    std::vector<uint8_t> new_packet;
    const uint8_t       *nal_start, *nal_end, *nal_codestart;
    const uint8_t       *end = packet + size;
    int                  type;

    nal_start = avc_find_startcode(packet, end);

    int startPos = nal_start - packet;

    nal_end = nullptr;
    while (nal_end != end) {
        nal_codestart = nal_start;

        while (nal_start < end && !*(nal_start++))
            ;

        if (nal_start == end) break;

        type = nal_start[0] & 0x1F;

        nal_end = avc_find_startcode(nal_start, end);
        if (!nal_end) nal_end = end;

        size_t s = (nal_end - nal_codestart);

        if (type == OBS_NAL_SPS || type == OBS_NAL_PPS) {
            LOG << "OBS_NAL_SPS: new size " << s;
            if (tp == OBS_ENCODER_VIDEO) {
                m_videoHeader.resize(m_videoHeader.size() + s);
                LOG << "writing video header m_videoHeader.size() " << m_videoHeader.size();
                memcpy(&m_videoHeader[m_videoHeader.size() - s], nal_codestart, s);
            }

        } else if (type == OBS_NAL_SEI) {
            LOG << "OBS_NAL_SEI: new size " << s;
            m_sei.resize(m_sei.size() + s);
            memcpy(&m_sei[m_sei.size() - s], nal_codestart, s);
        } else {
            LOG << "new size " << s;
            new_packet.resize(new_packet.size() + s);
            memcpy(&new_packet[new_packet.size() - s], nal_codestart, s);
        }

        nal_start = nal_end;
    }
    return std::move(new_packet);
}

bool RTMPSender::avc_keyframe(const uint8_t *data, size_t size) {
    const uint8_t *nal_start, *nal_end;
    const uint8_t *end = data + size;
    int            type;

    nal_start = avc_find_startcode(data, end);
    while (true) {
        while (nal_start < end && !*(nal_start++))
            ;

        if (nal_start == end) break;

        type = nal_start[0] & 0x1F;

        if (type == OBS_NAL_SLICE_IDR || type == OBS_NAL_SLICE) return (type == OBS_NAL_SLICE_IDR);

        nal_end   = avc_find_startcode(nal_start, end);
        nal_start = nal_end;
    }

    return false;
}

void RTMPSender::push_packet(int streamIdx, int64_t pts, int64_t dts, uint8_t *data, size_t size, obs_encoder_type tp) {
    unique_lock<mutex> l(m_stream.packets[m_stream.writePos].mutex);

    m_stream.packets[m_stream.writePos].pts      = pts;
    m_stream.packets[m_stream.writePos].dts      = dts;
    m_stream.packets[m_stream.writePos].type     = tp;
    m_stream.packets[m_stream.writePos].keyframe = avc_keyframe(data, size);
    m_stream.packets[m_stream.writePos].isHeader = tp == OBS_ENCODER_HEADER;

    if (m_stream.packets[m_stream.writePos].isHeader)
        LOG << "writing header to writePos " << m_stream.packets[m_stream.writePos].isHeader;

    if (m_stream.packets[m_stream.writePos].data.size() != size) m_stream.packets[m_stream.writePos].data.resize(size);

    memcpy(&m_stream.packets[m_stream.writePos].data[0], data, size);

    m_stream.writePos = ++m_stream.writePos % m_stream.packets.size();
}

bool RTMPSender::discard_recv_data(rtmp_stream &stream, size_t size) {
    if (!m_rtmp) return false;

    uint8_t buf[512];
#ifdef _WIN32
    int ret;
#else
    ssize_t ret;
#endif

    do {
        size_t bytes = size > 512 ? 512 : size;
        size -= bytes;

#ifdef _WIN32
        ret = recv(m_rtmp->m_sb.sb_socket, (char *)buf, (int)bytes, 0);
#else
        ret = recv(m_rtmp->m_sb.sb_socket, buf, bytes, 0);
#endif

        if (ret <= 0) {
#ifdef _WIN32
            int error = WSAGetLastError();
#else
            int error = errno;
#endif
            if (ret < 0) {
                LOGE << "recv error: " << error << " (" << (int)size << " bytes)";
            }
            return false;
        }
    } while (size > 0);

    return true;
}

/*
void RTMPSender::send_thread()
{
    if (!m_rtmp) {
        LOGE << "send_thread start failed";
        return;
    }

    int one = 1;
#ifdef _WIN32
    if (ioctlsocket(m_rtmp->m_sb.sb_socket, FIONBIO, &one))
    {
        m_rtmp->last_error_code = WSAGetLastError();
#else
    if (ioctl(stream->rtmp.m_sb.sb_socket, FIONBIO, &one))
    {
            m_rtmp->last_error_code = errno;
#endif
        LOGE << "Failed to set non-blocking socket";
        return;
    }

    LOG << "New socket loop enabled by user";

    if (m_low_latency_mode)
        LOG << "Low latency mode enabled by user";

    if (stream->write_buf)
        free(stream->write_buf);

    int total_bitrate = 0;

    obs_encoder_t *vencoder = obs_output_get_video_encoder(context);
    if (vencoder)
    {
        obs_data_t *params = obs_encoder_get_settings(vencoder);
        if (params)
        {
            int bitrate = obs_data_get_int(params, "bitrate");
            if (!bitrate) {
                LOGE << "Video decoder didn't return a valid bitrate, new
network code may function poorly. Low latency mode disabled.";
                m_low_latency_mode = false;
                bitrate = 10000;
            }
            total_bitrate += bitrate;
            obs_data_release(params);
        }
    }

    obs_encoder_t *aencoder = obs_output_get_audio_encoder(context, 0);
    if (aencoder)
    {
        obs_data_t *params = obs_encoder_get_settings(aencoder);
        if (params)
        {
            int bitrate = obs_data_get_int(params, "bitrate");
            if (!bitrate)
                bitrate = 160;
            total_bitrate += bitrate;
            obs_data_release(params);
        }
    }

    // to bytes/sec
    int ideal_buffer_size = total_bitrate * 128;

    if (ideal_buffer_size < 131072)
        ideal_buffer_size = 131072;

    stream->write_buf_size = ideal_buffer_size;
    stream->write_buf = malloc(ideal_buffer_size);

#ifdef _WIN32
    ret = pthread_create(&stream->socket_thread, NULL, socket_thread_windows,
stream); #else LOGE <<  "New socket loop not supported on this platform";
    return;
#endif

    if (ret != 0) {
        RTMP_Close(&stream->rtmp);
        warn("Failed to create socket thread");
        return OBS_OUTPUT_ERROR;
    }

    stream->socket_thread_active = true;
    m_rtmp->m_bCustomSend = true;
    m_rtmp->m_customSendFunc = socket_queue_data;
    m_rtmp->m_customSendParam = stream;
    }

    stream->active = true;

    if (!send_meta_data(stream)) {
        warn("Disconnected while attempting to send metadata");
        set_output_error(stream);
        return OBS_OUTPUT_DISCONNECTED;
    }

    obs_encoder_t *aencoder = obs_output_get_audio_encoder(context, 1);
    if (aencoder && !send_additional_meta_data(stream)) {
        warn("Disconnected while attempting to send additional "
             "metadata");
        return OBS_OUTPUT_DISCONNECTED;
    }

    if (obs_output_get_audio_encoder(context, 2) != NULL) {
        warn("Additional audio streams not supported");
        return OBS_OUTPUT_DISCONNECTED;
    }

    obs_output_begin_data_capture(stream->output, 0);
}*/

void RTMPSender::stop() {}

#ifdef _WIN32
void RTMPSender::win32_log_interface_type(RTMP *rtmp) {
    if (!rtmp) return;

    MIB_IPFORWARDROW route;
    uint32_t         dest_addr, source_addr;
    char             hostname[256];
    HOSTENT         *h;

    if (rtmp->Link.hostname.av_len >= sizeof(hostname) - 1) return;

    strncpy(hostname, rtmp->Link.hostname.av_val, sizeof(hostname));
    hostname[rtmp->Link.hostname.av_len] = 0;

    h = gethostbyname(hostname);
    if (!h) return;

    dest_addr = *(uint32_t *)h->h_addr_list[0];

    if (rtmp->m_bindIP.addrLen == 0)
        source_addr = 0;
    else if (rtmp->m_bindIP.addr.ss_family == AF_INET)
        source_addr = (*(struct sockaddr_in *)&rtmp->m_bindIP.addr).sin_addr.S_un.S_addr;
    else
        return;

    if (!GetBestRoute(dest_addr, source_addr, &route)) {
        MIB_IFROW row;
        memset(&row, 0, sizeof(row));
        row.dwIndex = route.dwForwardIfIndex;

        if (!GetIfEntry(&row)) {
            uint32_t    speed = row.dwSpeed / 1000000;
            std::string type;

            if (row.dwType == IF_TYPE_ETHERNET_CSMACD) {
                type = "ethernet";
            } else if (row.dwType == IF_TYPE_IEEE80211) {
                type = "802.11";
            } else {
                type = "type " + std::to_string(row.dwType);
            }

            LOG << "Interface: " << row.bDescr << " (" << type << ", " << speed << " mbps)";
        }
    }
}
#endif

inline bool RTMPSender::has_start_code(const uint8_t *data) {
    if (data[0] != 0 || data[1] != 0) return false;

    return data[2] == 1 || (data[2] == 0 && data[3] == 1);
}

void RTMPSender::get_sps_pps(const uint8_t *data, size_t size, const uint8_t **sps, size_t *sps_size,
                             const uint8_t **pps, size_t *pps_size) {
    const uint8_t *nal_start, *nal_end;
    const uint8_t *end = data + size;
    int            type;

    nal_start = avc_find_startcode(data, end);
    while (true) {
        while (nal_start < end && !*(nal_start++))
            ;

        if (nal_start == end) break;

        nal_end = avc_find_startcode(nal_start, end);

        type = nal_start[0] & 0x1F;
        if (type == OBS_NAL_SPS) {
            *sps      = nal_start;
            *sps_size = nal_end - nal_start;
        } else if (type == OBS_NAL_PPS) {
            *pps      = nal_start;
            *pps_size = nal_end - nal_start;
        }

        nal_start = nal_end;
    }
}

}  // namespace ara

#endif