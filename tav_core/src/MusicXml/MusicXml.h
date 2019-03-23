/*
 * MusicXml.h
 *
 *  Created on: Jun 13, 2018
 *      Author: sven
 */

#ifndef SRC_MUSICXML_H_
#define SRC_MUSICXML_H_

#include <cstring>
#include <signal.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>

#include <libmusicxml/xml.h>
#include <libmusicxml/xmlfile.h>
#include <libmusicxml/xmlreader.h>
#include <libmusicxml/midicontextvisitor.h>
#include <libmusicxml/xml_tree_browser.h>
#include <libmusicxml/partlistvisitor.h>
#include <libmusicxml/partsummary.h>
#include <libmusicxml/metronomevisitor.h>
#include <thread>
#include <chrono>

namespace tav
{

typedef struct {
	long 		 				date;
	unsigned int				channel;
	float						pitch;
	unsigned int				velocity;
	unsigned int				duration;
} musicXmlSeqEvent;

typedef struct {
	MusicXML2::ctree<MusicXML2::xmlelement>::iterator 	it;
	std::string											id;
	std::string											inst_name;
	std::vector<musicXmlSeqEvent>						eventList;
	unsigned int										eventPtr;
	unsigned int										measurePtr;
	float												minPitch;
	float												maxPitch;
	unsigned int										endDate;
	std::function<void(int, float, double)>	 			noteCbFunc;
	MusicXML2::S_part					 				part;
} musicXmlTrack;

//----------------------------------------------------

class mymidiwriter : public MusicXML2::midiwriter {
	public:
				 mymidiwriter(std::vector<musicXmlTrack>* _trackCol) : trackCol(_trackCol) {}
		virtual ~mymidiwriter() {}

		virtual void startPart (int instrCount) {}
		virtual void newInstrument (std::string instrName, int chan=-1) {

			//std::cout << " newInstrumentm, name: " << instrName << std::endl;

			trackCol->push_back(musicXmlTrack());
			trackCol->back().inst_name = instrName;
			trackCol->back().maxPitch = 0;
			trackCol->back().minPitch = 256;
			trackCol->back().maxPitch = 0.f;
			trackCol->back().minPitch = 255.f;
			trackCol->back().eventPtr = 0;
		}

		virtual void newNote (long date, int chan, float pitch, int vel, int dur) {

			trackCol->back().eventList.push_back(musicXmlSeqEvent());
			trackCol->back().eventList.back().date = date;
			trackCol->back().eventList.back().channel = chan;
			trackCol->back().eventList.back().pitch = pitch;
			trackCol->back().eventList.back().velocity = vel;
			trackCol->back().eventList.back().duration = dur;
			if (pitch > trackCol->back().maxPitch) trackCol->back().maxPitch = pitch;
			if (pitch < trackCol->back().minPitch) trackCol->back().minPitch = pitch;

/*			printf( "newNote[%d]: date:%ld chan:%d, pitch:%f vel:%d dur:%d maxPitch: %f, minPitch: %f\n",
					trackCol->back().eventList.size() -1 , trackCol->back().eventList.back().date,
					trackCol->back().eventList.back().channel, trackCol->back().eventList.back().pitch,
					trackCol->back().eventList.back().velocity, trackCol->back().eventList.back().duration,
					trackCol->back().maxPitch, trackCol->back().minPitch); */
		}

		virtual void endPart (long date)
		{
			/*
			std::cout << "setting event ptr, trackCol->back().eventList.size()" << trackCol->back().eventList.size() << std::endl;
			std::cout << "  ttrackCol->back().maxPitch:" << trackCol->back().maxPitch;
			std::cout << "  ttrackCol->back().minPitch:" << trackCol->back().minPitch << std::endl; */
			trackCol->back().endDate = date;
			trackCol->back().eventPtr = 0;
			trackCol->back().measurePtr = trackCol->back().eventList[0].date;
		}

		virtual void volChange (long date, int chan, int vol) { cout << date << " volChange chan  " << chan << " vol " << vol << endl; }
		virtual void tempoChange (long date, int bpm) { cout << "tempoChange" << endl; }
		virtual void progChange (long date, int chan, int prog) {  }
		virtual void bankChange (long date, int chan, int bank) {  }
		virtual void pedalChange (long date, pedalType t, int value) { }
private:
	std::vector<musicXmlTrack>*		trackCol;
	musicXmlTrack*					actTrackPtr;
};

//----------------------------------------------------

class MusicXml
{
public:

	MusicXml();
	virtual ~MusicXml();

	void read(const char* _fileName);
	void close();
	void setNoteCallbackFunction(std::function<void(int, float, double)> _func);

	inline void play() {
		actTime = 0.0;
		isPlaying = true;
		waitInt = 16;

		std::cout << "MusicXMl start playing" << std::endl;
		std::thread t([&] {
			while (isPlaying){
				tick(0.001 * (float)waitInt);
				std::chrono::milliseconds sleepFor(waitInt);
			    std::this_thread::sleep_for(sleepFor);
			}
		});
		t.detach();
	}
	inline void stop() { isPlaying = false; }

	void tick(double dt);

	inline std::string* getSongName() { return &songName; }
	inline unsigned int getNrTracks() { return (unsigned int) tracks.size(); }
	inline float getTrackMinPitch(int trackNr) { return tracks[trackNr].minPitch; }
	inline float getTrackMaxPitch(int trackNr) { return tracks[trackNr].maxPitch; }

	bool playing(){ return isPlaying; };
private :
	bool									debug;
	bool									isPlaying;

	const char*								fileName;
	unsigned int							tpq;
	unsigned int							bpm;
	long									actMeasure;

	double									actTime;
	int										waitInt;

	MusicXML2::xmlreader					reader;
	MusicXML2::SXMLFile						file;
	MusicXML2::Sxmlelement					st;

	std::string								songName;
	std::vector<musicXmlTrack>				tracks;
};

} /* namespace tav */

#endif /* SRC_MUSICXML_H_ */
