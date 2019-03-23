/*
 * VideoTextureGStreamer.cpp
 *
 *  Created on: Jul 18, 2018
 *      Author: sven
 */

#include "VideoTextureGStreamer.h"

namespace tav
{

VideoTextureGStreamer::VideoTextureGStreamer()
{
	gst_init (NULL, NULL);
}

//---------------------------------------------------------------------------

void VideoTextureGStreamer::openFile(std::string _file)
{
	actVideoFile = _file;
}

//---------------------------------------------------------------------------

// main thread command while loop
void VideoTextureGStreamer::play()
{
	std::cout << "doing play" << std::endl;

	if (!m_loop_thread)
	{
		if (actVideoFile.length())
		{
			if((m_loop_thread = g_thread_new("main_thread",
					(GThreadFunc) &VideoTextureGStreamer::start_gst_main_loop,
					NULL)) == NULL)
				std::cerr << "GstPlayer() couldn't create thread" << std::endl;

		} else  std::cerr << "actualFile not set!!!, No file selected" << std::endl;

	} else
	{
		gst_element_get_state(data.playbin, &state, NULL, GST_CLOCK_TIME_NONE);

		if (state == GST_STATE_PAUSED)
			ret = gst_element_set_state (data.playbin, GST_STATE_PLAYING);
	}
}

//---------------------------------------------------------------------------

void VideoTextureGStreamer::pause()
{
	std::cout << "doing pause" << std::endl;

	gst_element_get_state(data.playbin, &state, NULL, GST_CLOCK_TIME_NONE);

	if (state == GST_STATE_PLAYING)
	{
		ret = gst_element_set_state (data.playbin, GST_STATE_PAUSED);
		if (ret == GST_STATE_CHANGE_FAILURE)
			std::cerr << "Unable to set the pipeline to the playing state." << std::endl;
	}
}

//---------------------------------------------------------------------------

void VideoTextureGStreamer::stop()
{
	std::cout << "doing stop, m_loop_thread:" << m_loop_thread << std::endl;

	gst_element_get_state(data.playbin, &state, NULL, GST_CLOCK_TIME_NONE);

	if (m_loop_thread)
	{
		if (state == GST_STATE_PAUSED){
			ret = gst_element_set_state (data.playbin, GST_STATE_PLAYING);
			usleep(100);
		}

		//print_pos = false;

		std::cout << "stopping from play, set eos to playbin" << std::endl;
		gst_element_send_event(data.playbin, gst_event_new_eos());

		//g_main_loop_quit(data.main_loop); // seg fault...

		g_thread_join(m_loop_thread);
		std::cerr << "g_thread_join joined." << std::endl;

		m_loop_thread = 0;
	}
}

//---------------------------------------------------------------------------

/*
// Process a color balance command
void VideoTextureGStreamer::update_color_channel (const gchar *channel_name, float value, GstColorBalance *cb)
{
	gdouble step;
	GstColorBalanceChannel *channel = NULL;
	const GList *channels, *l;

	// Retrieve the list of channels and locate the requested one
	channels = gst_color_balance_list_channels (cb);

	for (l = channels; l != NULL; l = l->next)
	{
		GstColorBalanceChannel *tmp = (GstColorBalanceChannel *)l->data;

		if (g_strrstr (tmp->label, channel_name)) {
			channel = tmp;
			break;
		}
	}
	if (!channel)
		return;

	gint act_value = gst_color_balance_get_value (cb, channel);

	//std::cout << "act_value" << act_value << " channel->max_value:" << channel->max_value << " channel->min_value:" << channel->min_value << std::endl;

	gst_color_balance_set_value (cb, channel, (gint)((float)(channel->max_value - channel->min_value) * value) + channel->min_value);
}

//---------------------------------------------------------------------------

// Output the current values of all Color Balance channels
void VideoTextureGStreamer::print_current_values (GstElement *pipeline)
{
	const GList *channels, *l;

	// Output Color Balance values
	channels = gst_color_balance_list_channels (GST_COLOR_BALANCE (pipeline));
	for (l = channels; l != NULL; l = l->next) {
		GstColorBalanceChannel *channel = (GstColorBalanceChannel *)l->data;
		gint value = gst_color_balance_get_value (GST_COLOR_BALANCE (pipeline), channel);
		g_print ("%s: %3d%% ", channel->label,
				100 * (value - channel->min_value) / (channel->max_value - channel->min_value));
	}
	g_print ("\n");
}
*/

//---------------------------------------------------------------------------

// Extract some metadata from the streams and print it on the screen
void VideoTextureGStreamer::analyze_streams (CustomData *data)
{
	gint i;
	GstTagList *tags;
	gchar *str;
	guint rate;

	// Read some properties
	g_object_get (data->playbin, "n-video", &data->n_video, NULL);
	g_object_get (data->playbin, "n-audio", &data->n_audio, NULL);
	g_object_get (data->playbin, "n-text", &data->n_text, NULL);

	g_print ("%d video stream(s), %d audio stream(s), %d text stream(s)\n",
			data->n_video, data->n_audio, data->n_text);

	g_print ("\n");
	for (i = 0; i < data->n_video; i++) {
		tags = NULL;
		// Retrieve the stream's video tags
		g_signal_emit_by_name (data->playbin, "get-video-tags", i, &tags);
		if (tags) {
			g_print ("video stream %d:\n", i);
			gst_tag_list_get_string (tags, GST_TAG_VIDEO_CODEC, &str);
			g_print ("  codec: %s\n", str ? str : "unknown");
			g_free (str);
			gst_tag_list_free (tags);
		}
	}

	g_print ("\n");
	for (i = 0; i < data->n_audio; i++) {
		tags = NULL;
		// Retrieve the stream's audio tags
		g_signal_emit_by_name (data->playbin, "get-audio-tags", i, &tags);
		if (tags) {
			g_print ("audio stream %d:\n", i);
			if (gst_tag_list_get_string (tags, GST_TAG_AUDIO_CODEC, &str)) {
				g_print ("  codec: %s\n", str);
				g_free (str);
			}
			if (gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &str)) {
				g_print ("  language: %s\n", str);
				g_free (str);
			}
			if (gst_tag_list_get_uint (tags, GST_TAG_BITRATE, &rate)) {
				g_print ("  bitrate: %d\n", rate);
			}
			gst_tag_list_free (tags);
		}
	}

	g_print ("\n");
	for (i = 0; i < data->n_text; i++) {
		tags = NULL;
		// Retrieve the stream's subtitle tags
		g_signal_emit_by_name (data->playbin, "get-text-tags", i, &tags);
		if (tags) {
			g_print ("subtitle stream %d:\n", i);
			if (gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &str)) {
				g_print ("  language: %s\n", str);
				g_free (str);
			}
			gst_tag_list_free (tags);
		}
	}

	g_object_get (data->playbin, "current-video", &data->current_video, NULL);
	g_object_get (data->playbin, "current-audio", &data->current_audio, NULL);
	g_object_get (data->playbin, "current-text", &data->current_text, NULL);

	g_print ("\n");
	g_print ("Currently playing video stream %d, audio stream %d and text stream %d\n",
			data->current_video, data->current_audio, data->current_text);
	g_print ("Type any number and hit ENTER to select a different audio stream\n");
}

//---------------------------------------------------------------------------

// Process messages from GStreamer
gboolean VideoTextureGStreamer::handle_message (GstBus *bus, GstMessage *msg, CustomData *data)
{
	GError *err;
	gchar *debug_info;

	switch (GST_MESSAGE_TYPE (msg))
	{
	case GST_MESSAGE_ERROR:
		gst_message_parse_error (msg, &err, &debug_info);
		g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
		g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
		g_clear_error (&err);
		g_free (debug_info);
		g_main_loop_quit (data->main_loop);
		break;

	case GST_MESSAGE_EOS:
		g_print ("End-Of-Stream reached.\n");
		g_main_loop_quit (data->main_loop);
		break;

	case GST_MESSAGE_STATE_CHANGED:
	{
		GstState old_state, new_state, pending_state;
		gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);

		if (data && data->playbin)
		{
			if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->playbin))
			{
				if (new_state == GST_STATE_PLAYING)
				{
					std::cout << " new_state == GST_STATE_PLAYING -> analyze_streams" << std::endl;

					// Once we are in the playing state, analyze the streams
					analyze_streams (data);
				}

				if (new_state == GST_STATE_NULL)
				{
					std::cout << " stopped" << std::endl;
				}
			}
		}
	} break;
	}

	// We want to keep receiving messages
	return TRUE;
}

//---------------------------------------------------------------------------
/*
// Process keyboard input
gboolean VideoTextureGStreamer::handle_keyboard (GIOChannel *source, GIOCondition cond, CustomData *data)
{
	gchar *str = NULL;

	if (g_io_channel_read_line (source, &str, NULL, NULL, NULL) == G_IO_STATUS_NORMAL)
	{
		int index = g_ascii_strtoull (str, NULL, 0);

		if (index < 0 || index >= data->n_audio) {
			g_printerr ("Index out of bounds\n");
		} else {
			// If the input was a valid audio stream index, set the current audio stream
			g_print ("Setting current audio stream to %d\n", index);
			g_object_set (data->playbin, "current-audio", index, NULL);
		}
	}

	g_free (str);
	return TRUE;
}
*/
//-----------------------------------------------------------------------------------

void VideoTextureGStreamer::seek_to_time(GstElement *pipeline, gint64 time_nanoseconds)
{
	if (!gst_element_seek(pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                         	 GST_SEEK_TYPE_SET, time_nanoseconds,
							 GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
	{
		g_print ("Seek failed!\n");
	}
}

//-----------------------------------------------------------------------------------

gboolean VideoTextureGStreamer::cb_print_position (GstElement *pipeline)
{
	gint64 pos, len;
	char time_chars [128];

	if (gst_element_query_position (pipeline, GST_FORMAT_TIME, &pos)
		&& gst_element_query_duration (pipeline, GST_FORMAT_TIME, &len))
	{
		sprintf(time_chars, "[{\"pos\":\"%" GST_TIME_FORMAT "\",\"len\":\"%" GST_TIME_FORMAT "\"}]", GST_TIME_ARGS (pos), GST_TIME_ARGS (len));
		//g_print ("Time: %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
		//		GST_TIME_ARGS (pos), GST_TIME_ARGS (len));
	}

	return true;
}

//-----------------------------------------------------------------------------------

int VideoTextureGStreamer::start_gst_main_loop(void)
{
	std::cout << "start_gst_main_loop" << std::endl;

	// Create the elements
	data.playbin = gst_element_factory_make ("playbin", "playbin");
	if (!data.playbin) {
		g_printerr ("Not all elements could be created.\n");
	}
/*

	// Add a bus watch, so we get notified when a message arrives
	bus = gst_element_get_bus (data.playbin);
	gst_bus_add_watch (bus, (GstBusFunc)&VideoTextureGStreamer::handle_message, &data);

	// Set connection speed. This will affect some internal decisions of playbin
	//g_object_set (data.playbin, "connection-speed", 56, NULL);

	// Add a keyboard watch so we get notified of keystrokes
	//io_stdin = g_io_channel_unix_new (fileno (stdin));
	//g_io_add_watch (io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, &data);

	g_printerr("opening: %s\n", actVideoFile.c_str());

	// Set the URI to play
	g_object_set (data.playbin, "uri", actVideoFile.c_str(), NULL);

	// Set flags to show Audio and Video but ignore Subtitles
	g_object_get (data.playbin, "flags", &flags, NULL);
	flags |= GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO;
	flags &= ~GST_PLAY_FLAG_TEXT;
	g_object_set (data.playbin, "flags", flags, NULL);

	// add a video filter


	// Start playing
	ret = gst_element_set_state (data.playbin, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		std::cerr << "Unable to set the pipeline to the playing state." << std::endl;
		gst_object_unref (data.playbin);
	}

	// Create a GLib Main Loop and set it to run
//	print_pos = true;
	g_timeout_add (200, (GSourceFunc) cb_print_position, data.playbin);

	//print_current_values(data.playbin);

	data.main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run (data.main_loop);

	// ----- wait for end --------------------------

	gst_element_set_state(data.playbin, GST_STATE_NULL);

	std::cout << "g_main_loop_run returns" << std::endl;
	gst_object_unref (GST_OBJECT (data.playbin));

	// Free resources
	gst_object_unref (bus);

	std::cout << "return from main loop" << std::endl;
*/

	return 0;
}


//-----------------------------------------------------------------------------------

VideoTextureGStreamer::~VideoTextureGStreamer()
{
	// TODO Auto-generated destructor stub
}

} /* namespace tav */
