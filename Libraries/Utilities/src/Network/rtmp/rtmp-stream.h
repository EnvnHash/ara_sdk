#ifdef ARA_USE_LIBRTMP

#include <inttypes.h>

#include "Network/rtmp/RtmpUtils.h"
#include "Network/rtmp/librtmp/log.h"
#include "Network/rtmp/librtmp/rtmp.h"
#include "Network/rtmp/util/circlebuf.h"

// #include "flv-mux.h"
// #include "net-if.h"

#include "Conditional.h"
#include "Log.h"
#include "util_common.h"

#ifdef _WIN32
#include <iphlpapi.h>
#else

#include <sys/ioctl.h>

#endif

#define OPT_DYN_BITRATE "dyn_bitrate"
#define OPT_DROP_THRESHOLD "drop_threshold_ms"
#define OPT_PFRAME_DROP_THRESHOLD "pframe_drop_threshold_ms"
#define OPT_MAX_SHUTDOWN_TIME_SEC "max_shutdown_time_sec"
#define OPT_BIND_IP "bind_ip"
#define OPT_NEWSOCKETLOOP_ENABLED "new_socket_loop_enabled"
#define OPT_LOWLATENCY_ENABLED "low_latency_mode_enabled"
#define OPT_METADATA_MULTITRACK "metadata_multitrack"

// #define TEST_FRAMEDROPS
// #define TEST_FRAMEDROPS_WITH_BITRATE_SHORTCUTS

#ifdef TEST_FRAMEDROPS

#define DROPTEST_MAX_KBPS 3000
#define DROPTEST_MAX_BYTES (DROPTEST_MAX_KBPS * 1000 / 8)

struct droptest_info {
    uint64_t ts;
    size_t   size;
};
#endif

namespace ara {

struct dbr_frame {
    uint64_t send_beg;
    uint64_t send_end;
    size_t   size;
};

struct os_event_data;
typedef struct os_event_data os_event_t;

class rtmp_stream {
public:
    obs_output *output = nullptr;

    std::mutex packets_mutex;
    circlebuf  packets;
    bool       sent_headers;

    bool    got_first_video;
    int64_t start_dts_offset;

    std::atomic<bool> connecting = false;
    std::thread       connect_thread;

    std::atomic<bool> active       = false;
    std::atomic<bool> disconnected = true;
    std::atomic<bool> encode_error = false;
    std::thread       send_thread;

    int max_shutdown_time_sec;

    Conditional send_sem;
    Conditional stop_event;
    uint64_t    stop_ts             = 0;
    uint64_t    shutdown_timeout_ts = 0;

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

    std::mutex dbr_mutex;
    circlebuf  dbr_frames;
    size_t     dbr_data_size    = 0;
    uint64_t   dbr_inc_timeout  = 0;
    long       audio_bitrate    = 0;
    long       dbr_est_bitrate  = 0;
    long       dbr_orig_bitrate = 0;
    long       dbr_prev_bitrate = 0;
    long       dbr_cur_bitrate  = 0;
    long       dbr_inc_bitrate  = 0;
    bool       dbr_enabled      = 0;

    RTMP rtmp;

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

static const char *rtmp_stream_getname(void *unused);

static void log_rtmp(int level, const char *format, va_list args);

static void free_packets(rtmp_stream *stream);

static void rtmp_stream_destroy(void *data);

static inline size_t num_buffered_packets(rtmp_stream *stream) { return stream->packets.size / sizeof(encoder_packet); }
static bool          stopping(rtmp_stream *stream) { return stream->stop_event.isNotified(); }
static bool          connecting(rtmp_stream *stream) { return stream->connecting; }
static bool          active(rtmp_stream *stream) { return stream->active; }
static bool          disconnected(rtmp_stream *stream) { return stream->disconnected; }

#ifdef _WIN32
void *socket_thread_windows(void *data);
#endif
}  // namespace ara

#endif