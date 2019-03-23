/*
 * MusicXml.cpp
 *
 *  Created on: Jun 13, 2018
 *      Author: sven
 *
 *  MusicXML format:
 *  root element: <score-partwise>
 *  1. child: <movement-title> -> SongName
 *  2. child <identification> -> Program that was used to generate this file
 *  3. child <defaults>
 *  	<scaling> -> resolution of the score page
 *  	<page-layout> -> width, height, border (print layout parameters)
 *		<system-layout> -> print layout paramenters
 *		<staff-layout> -> print layout paramenters
 *	4. child <part-list>
 *		<part-group>
 *		...
 *		<part-group>
 *		<score-part id=""> -> the different tracks when exported from logic
 *  5. child <part>
 *  	<measure>
 *  		<attributes>
 *  		<note>
 */

#include "MusicXml.h"

using namespace std;
using namespace MusicXML2;

namespace tav
{

MusicXml::MusicXml() : fileName(NULL), debug(false), file(NULL), tpq(480),
		isPlaying(false), bpm(114.82)
{}

//----------------------------------------------------

void MusicXml::read(const char* _fileName)
{
	fileName = _fileName;
	file = reader.read(_fileName);

	if (file)
	{
		st = file->elements();	// root node <score-partwise>
		ctree<xmlelement>::iterator iter = st->begin();

		// get songName
		songName = (*iter)->getValue();
		if(debug) std::cout << "MusicXml::read: SongName: " << songName << std::endl;
		if(debug) std::cout << "now  convert the parts into event lists "  << std::endl;

		// now convert the parts into event lists
		mymidiwriter writer = mymidiwriter(&tracks);
		midicontextvisitor v = midicontextvisitor(tpq, &writer);	// first paramenter tpq (time per quarter);
		xml_tree_browser midi_browser(&v);
		midi_browser.browse(*st);

		//std::cout << "Song Duration: " << v.getDuration() << endl;

		// now we have got the track information and the separate <part>s per track with the notes
		// we are done!

	} else {
		std::cerr << "MusicXml::read Error in file " << _fileName <<  " probably an XML syntax error" << std::endl;
	}
}

//----------------------------------------------------

void MusicXml::close()
{
	tracks.clear();
	tracks.resize(0);
}

//----------------------------------------------------

void MusicXml::setNoteCallbackFunction(std::function<void(int, float, double)> _func)
{
	for(std::vector<musicXmlTrack>::iterator it = tracks.begin(); it != tracks.end(); ++it ){
		(*it).noteCbFunc = _func;
	}
}

//----------------------------------------------------

void MusicXml::tick(double dt)
{
	if (isPlaying)
	{
		actTime += dt;
		actMeasure = static_cast<long>((double)bpm / 60.0 * std::max(actTime, 0.0) * (double)tpq);

		//std::cout << "tick dt: " << dt << " actTime " << actTime;
		//std::cout << " actMeasure:" << actMeasure << std::endl;

		// iterate through all tracks
		for(std::vector<musicXmlTrack>::iterator it = tracks.begin(); it != tracks.end(); ++it )
		{
			// if the actual eventPtrs date is smaller than the actual measureTime, call note
			if ( (*it).measurePtr < actMeasure && (*it).eventPtr < ((*it).eventList.size()-1) )
			{
				(*it).noteCbFunc(it - tracks.begin(), (*it).eventList[(*it).eventPtr].pitch,
						(*it).eventList[(*it).eventPtr].duration);
				(*it).eventPtr++;
				(*it).measurePtr = (*it).eventList[(*it).eventPtr].date;
			}
		}
	}
}

//----------------------------------------------------

MusicXml::~MusicXml()
{
}

} /* namespace tav */
