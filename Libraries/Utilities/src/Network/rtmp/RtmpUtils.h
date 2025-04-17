//
// Created by user on 09.09.2021.
//

#ifdef ARA_USE_LIBRTMP

#pragma once

#define MAX_RETRY_SEC (15 * 60)

#define MAX_AUDIO_MIXES 6
#define MAX_AV_PLANES 8
#define MAX_AUDIO_CHANNELS 8
#define AUDIO_OUTPUT_FRAMES 1024

#define OBS_OUTPUT_SUCCESS 0
#define OBS_OUTPUT_BAD_PATH -1
#define OBS_OUTPUT_CONNECT_FAILED -2
#define OBS_OUTPUT_INVALID_STREAM -3
#define OBS_OUTPUT_ERROR -4
#define OBS_OUTPUT_DISCONNECTED -5
#define OBS_OUTPUT_UNSUPPORTED -6
#define OBS_OUTPUT_NO_SPACE -7
#define OBS_OUTPUT_ENCODE_ERROR -8

#define VIDEO_HEADER_SIZE 5
#define VIDEODATA_AVCVIDEOPACKET 7.0
#define AUDIODATA_AAC 10.0

#include "Conditional.h"
#include "Network/rtmp/util/circlebuf.h"
#include "util_common.h"

namespace ara {

class obs_data;

class obs_data_item {
public:
    std::atomic<long>  ref;
    obs_data          *parent = nullptr;
    obs_data_item     *next   = nullptr;
    enum obs_data_type type;
    size_t             name_len        = 0;
    size_t             data_len        = 0;
    size_t             data_size       = 0;
    size_t             default_len     = 0;
    size_t             default_size    = 0;
    size_t             autoselect_size = 0;
    size_t             capacity        = 0;
};

enum {
    OBS_NAL_UNKNOWN   = 0,
    OBS_NAL_SLICE     = 1,
    OBS_NAL_SLICE_DPA = 2,
    OBS_NAL_SLICE_DPB = 3,
    OBS_NAL_SLICE_DPC = 4,
    OBS_NAL_SLICE_IDR = 5,
    OBS_NAL_SEI       = 6,
    OBS_NAL_SPS       = 7,
    OBS_NAL_PPS       = 8,
    OBS_NAL_AUD       = 9,
    OBS_NAL_FILLER    = 12,
};

class obs_data {
public:
    std::atomic<long> ref;
    char             *json       = nullptr;
    obs_data_item    *first_item = nullptr;
};

enum video_colorspace {
    VIDEO_CS_DEFAULT,
    VIDEO_CS_601,
    VIDEO_CS_709,
    VIDEO_CS_SRGB,
};

enum video_range_type { VIDEO_RANGE_DEFAULT, VIDEO_RANGE_PARTIAL, VIDEO_RANGE_FULL };

enum obs_encoder_type {
    OBS_ENCODER_AUDIO = 0, /**< The decoder provides an audio codec */
    OBS_ENCODER_VIDEO,     /**< The decoder provides a video codec */
    OBS_ENCODER_HEADER     /**< The decoder provides a video codec */
};

struct video_output;
typedef struct video_output video_t;

struct audio_output;
typedef struct audio_output audio_t;

class pause_data {
public:
    std::mutex mutex;
    uint64_t   last_video_ts;
    uint64_t   ts_start;
    uint64_t   ts_end;
    uint64_t   ts_offset;
};

struct video_scale_info {
    enum video_format     format;
    uint32_t              width;
    uint32_t              height;
    enum video_range_type range;
    enum video_colorspace colorspace;
};

struct audio_convert_info {
    uint32_t            samples_per_sec;
    enum audio_format   format;
    enum speaker_layout speakers;
};

/** Encoder output packet */
class encoder_packet {
public:
    std::vector<uint8_t> data; /**< Packet data */
    // size_t size;   /**< Packet size  ...redundant is same as data.size()

    int64_t pts = 0; /**< Presentation timestamp */
    int64_t dts = 0; /**< Decode timestamp */

    int64_t duration = 0;
    int64_t pos      = 0;  ///< byte position in stream, -1 if unknown

    int32_t timebase_num = 0; /**< Timebase numerator */
    int32_t timebase_den = 1; /**< Timebase denominator */

    enum obs_encoder_type type; /**< Encoder type */

    bool keyframe = false; /**< Is a keyframe */
    bool isHeader = false; /**< Is a keyframe */

    /** Audio track index (used with outputs) */
    size_t track_idx = 0;

    std::mutex mutex;
};

class dbr_frame {
public:
    uint64_t send_beg = 0;
    uint64_t send_end = 0;
    size_t   size     = 0;
};

class obs_output;  // forware declaration

class rtmp_stream {
public:
    obs_output *output = nullptr;

    std::mutex                     packets_mutex;
    std::array<encoder_packet, 16> packets;
    size_t                         writePos = 0;
    size_t                         readPos  = 0;

    bool    sent_headers;
    bool    got_first_video;
    int64_t start_dts_offset;

    std::atomic<bool> connecting = false;
    std::thread       connect_thread;

    std::atomic<bool> active       = false;
    std::atomic<bool> disconnected = true;
    std::atomic<bool> encode_error = false;
    std::thread       send_thread;

    int max_shutdown_time_sec;

    Conditional       send_sem;
    std::atomic<bool> stopping            = false;
    uint64_t          stop_ts             = 0;
    uint64_t          shutdown_timeout_ts = 0;

    std::string path = {0}, key = {0};
    std::string username = {0}, password = {0};
    std::string encoder_name = {0};
    std::string bind_ip      = {0};

    // frame drop variables
    int64_t drop_threshold_use         = 0;
    int64_t pframe_drop_threshold_usec = 0;
    int     min_priority               = 0;
    float   congestion                 = 0.f;

    int64_t last_dts_usec = 0;

    uint64_t total_bytes_sent = 0;
    int      dropped_frames   = 0;

#ifdef TEST_FRAMEDROPS
    circlebuf droptest_info;
    uint64_t  droptest_last_key_check;
    size_t    droptest_max;
    size_t    droptest_size;
#endif

    std::mutex  dbr_mutex;
    circlebuf   dbr_frames;
    size_t      dbr_data_size                    = 0;
    uint64_t    dbr_inc_timeout                  = 0;
    long        audio_bitrate                    = 0;
    long        dbr_est_bitrate                  = 0;
    long        dbr_orig_bitrate                 = 0;
    long        dbr_prev_bitrate                 = 0;
    long        dbr_cur_bitrate                  = 0;
    long        dbr_inc_bitrate                  = 0;
    bool        dbr_enabled                      = 0;
    bool        new_socket_loop                  = false;
    bool        low_latency_mode                 = false;
    bool        disable_send_window_optimization = false;
    bool        socket_thread_active             = false;
    std::thread socket_thread;
    uint8_t    *write_buf      = nullptr;
    size_t      write_buf_len  = 0;
    size_t      write_buf_size = 0;
    std::mutex  write_buf_mutex;
    Conditional buffer_space_available_event;
    Conditional buffer_has_data_event;
    Conditional socket_available_event;
    Conditional send_thread_signaled_exit;
};

class obs_output {
public:
    //    struct obs_context_data context;
    //    struct obs_output_info info;
    //    struct obs_weak_output *control;

    // indicates ownership of the info.id buffer
    bool owns_info_id;

    bool                        received_video;
    bool                        received_audio;
    std::atomic<bool>           data_active;
    std::atomic<bool>           end_data_capture_thread_active;
    int64_t                     video_offset;
    int64_t                     audio_offsets[MAX_AUDIO_MIXES];
    int64_t                     highest_audio_ts;
    int64_t                     highest_video_ts;
    std::thread                 end_data_capture_thread;
    Conditional                 stopping_event;
    std::mutex                  interleaved_mutex;
    std::vector<encoder_packet> interleaved_packets;
    int                         stop_code;

    int               reconnect_retry_sec;
    int               reconnect_retry_max;
    int               reconnect_retries;
    int               reconnect_retry_cur_sec;
    std::thread       reconnect_thread;
    Conditional       reconnect_stop_event;
    std::atomic<bool> reconnecting;
    std::atomic<bool> reconnect_thread_active;

    uint32_t starting_drawn_count;
    uint32_t starting_lagged_count;
    uint32_t starting_frame_count;

    int total_frames;

    std::atomic<bool> active;
    std::atomic<bool> paused;
    video_t          *video;
    audio_t          *audio;
    // obs_encoder_t *video_encoder;
    // obs_encoder_t *audio_encoders[MAX_AUDIO_MIXES];
    // obs_service_t *service;
    size_t mixer_mask;

    pause_data pause;

    circlebuf audio_buffer[MAX_AUDIO_MIXES][MAX_AV_PLANES];
    uint64_t  audio_start_ts;
    uint64_t  video_start_ts;
    size_t    audio_size;
    size_t    planes;
    size_t    sample_rate;
    size_t    total_audio_frames;

    uint32_t scaled_width;
    uint32_t scaled_height;

    bool                      video_conversion_set;
    bool                      audio_conversion_set;
    struct video_scale_info   video_conversion;
    struct audio_convert_info audio_conversion;

    std::mutex           caption_mutex;
    double               caption_timestamp;
    struct caption_text *caption_head;
    struct caption_text *caption_tail;

    circlebuf caption_data;

    bool valid;

    uint64_t              active_delay_ns;
    std::function<void()> delay_callback;
    circlebuf             delay_data; /* struct delay_data */
    std::mutex            delay_mutex;
    uint32_t              delay_sec;
    uint32_t              delay_flags;
    uint32_t              delay_cur_flags;
    std::atomic<long>     delay_restart_refs;
    std::atomic<bool>     delay_active;
    std::atomic<bool>     delay_capturing;

    char *last_error_message;

    float audio_data[MAX_AUDIO_CHANNELS][AUDIO_OUTPUT_FRAMES];
};

static void push_wb16(std::vector<uint8_t> &v, unsigned int val) {
    v.emplace_back((int)val >> 8);
    v.emplace_back((uint8_t)val);
}

static void push_wb24(std::vector<uint8_t> &v, unsigned int val) {
    push_wb16(v, (int)val >> 8);
    v.emplace_back((uint8_t)val);
}

static void push_wb32(std::vector<uint8_t> &v, unsigned int val) {
    v.emplace_back(val >> 24);
    v.emplace_back((uint8_t)(val >> 16));
    v.emplace_back((uint8_t)(val >> 8));
    v.emplace_back((uint8_t)val);
}

static void push_wb64(std::vector<uint8_t> &v, uint64_t u64) {
    push_wb32(v, (uint32_t)(u64 >> 32));
    push_wb32(v, (uint32_t)u64);
}

static void push_str(std::vector<uint8_t> &v, std::string str) { v.insert(v.end(), str.begin(), str.end()); }

static void s_u29(std::vector<uint8_t> &v, uint32_t val) {
    if (val <= 0x7F) {
        v.emplace_back(val);
    } else if (val <= 0x3FFF) {
        v.emplace_back(0x80 | (val >> 7));
        v.emplace_back(val & 0x7F);
    } else if (val <= 0x1FFFFF) {
        v.emplace_back(0x80 | (val >> 14));
        v.emplace_back(0x80 | ((val >> 7) & 0x7F));
        v.emplace_back(val & 0x7F);
    } else {
        v.emplace_back(0x80 | (val >> 22));
        v.emplace_back(0x80 | ((val >> 15) & 0x7F));
        v.emplace_back(0x80 | ((val >> 8) & 0x7F));
        v.emplace_back(val & 0xFF);
    }
}

static void s_u29b_value(std::vector<uint8_t> &v, uint32_t val) { s_u29(v, 1 | ((val & 0xFFFFFFF) << 1)); }

static bool can_reconnect(const obs_output *output, int code) {
    bool reconnect_active = output->reconnect_retry_max != 0;
    return (output->reconnecting && code != OBS_OUTPUT_SUCCESS) ||
           (reconnect_active && code == OBS_OUTPUT_DISCONNECTED);
}

void output_reconnect(obs_output *output);
bool obs_output_actual_start(obs_output *output);
void obs_output_signal_stop(obs_output *output, int code);
void obs_output_end_data_capture_internal(obs_output *output, bool signal);
void obs_output_end_data_capture(obs_output *output);

}  // namespace ara

#endif