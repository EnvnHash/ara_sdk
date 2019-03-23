/*
 * VideoTextureGStreamer.h
 *
 *  Created on: Jul 18, 2018
 *      Author: sven
 */

#ifndef SRC_VIDEOTEXTUREGSTREAMER_H_
#define SRC_VIDEOTEXTUREGSTREAMER_H_


#include <unistd.h>
#include <string>
#include <iostream>

#include <gst/gst.h>
#include <gst/video/colorbalance.h>


//#define BOOST_SPIRIT_THREADSAFE
//#include <boost/property_tree/json_parser.hpp>
//#include <boost/property_tree/ptree.hpp>

//using namespace boost::property_tree; // Added for the json-example:
//using namespace std::placeholders; // Namespace for _1, _2, ...

namespace tav
{


class VideoTextureGStreamer
{
public:

	// Structure to contain all our information, so we can pass it around
	typedef struct _CustomData {
		GstElement *playbin;  // Our one and only element

		gint n_video;          // Number of embedded video streams
		gint n_audio;          // Number of embedded audio streams
		gint n_text;           // Number of embedded subtitle streams

		gint current_video;    // Currently playing video stream
		gint current_audio;    // Currently playing audio stream
		gint current_text;     // Currently playing subtitle stream

		GMainLoop *main_loop;  // GLib's Main Loop
	} CustomData;

	// playbin flags
	typedef enum {
		GST_PLAY_FLAG_VIDEO         = (1 << 0), // We want video output
		GST_PLAY_FLAG_AUDIO         = (1 << 1), // We want audio output
		GST_PLAY_FLAG_TEXT          = (1 << 2)  // We want subtitle output
	} GstPlayFlags;

	typedef enum {
		CMD_IDLE         = (1 << 0),
		CMD_PLAY         = (1 << 1),
		CMD_STOP         = (1 << 2),
		CMD_PAUSE        = (1 << 3)
	} cmdMsg;


	VideoTextureGStreamer();
	virtual ~VideoTextureGStreamer();

	void openFile(std::string _file);

	void play();
	void pause();
	void stop();

	/*
	void update_color_channel (const gchar *channel_name, float value, GstColorBalance *cb);
	void print_current_values (GstElement *pipeline);*/

	void analyze_streams (CustomData *data);
	gboolean handle_message (GstBus *bus, GstMessage *msg, CustomData *data);
	//gboolean handle_keyboard (GIOChannel *source, GIOCondition cond, CustomData *data);
	void seek_to_time(GstElement *pipeline, gint64 time_nanoseconds);
	static gboolean cb_print_position (GstElement *pipeline);
	int start_gst_main_loop(void);
	static bool				print_pos;

private:
	std::string 			actVideoFile;

	CustomData				data;
	GstStateChangeReturn 	ret;
	GIOChannel*				io_stdin;
	GstBus*					bus;
	GstState 				state;
	GThread*				m_loop_thread=0;
	gint 					flags;
	cmdMsg					actCmd = CMD_IDLE;
	bool					run = true;

};

} /* namespace tav */

#endif /* SRC_VIDEOTEXTUREGSTREAMER_H_ */
