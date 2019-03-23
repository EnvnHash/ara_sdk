/*
 * SeqTrack.cpp
 *
 *  Created on: 22.03.2017
 *      Copyright by Sven Hahne
 */

#include "Sequencer/SeqTrack.h"
#include "Sequencer/SeqTracks.h"

namespace tav
{

SeqTrack::SeqTrack(xmlpp::Element* element, xmlpp::Document* _doc,
		unsigned int _id, SeqEvent* _metaEvent, SeqTracks* _parentTracks) :
		SeqElement(element, "track", _doc), metaEvent(_metaEvent), parentTracks(
				_parentTracks)
{
	// create single value sub nodes
	muteElement = createNewElement(t_element, "mute");
	soloElement = createNewElement(t_element, "solo");
	nameElement = createNewElement(t_element, "name");

	classBaseXMLTemplate = new SeqElementTempl(get_name().c_str());

	using std::placeholders::_1;
	classBaseXMLTemplate->addArg("id", std::bind(&SeqTrack::getName, this),
			std::bind(&SeqTrack::setId, this, _1));

	classBaseXMLTemplate->addElement("name",
			std::bind(&SeqTrack::getName, this),
			std::bind(&SeqTrack::setName, this, _1));
	classBaseXMLTemplate->addElement("mute",
			std::bind(&SeqTrack::getMute, this),
			std::bind(&SeqTrack::setMute, this, _1));
	classBaseXMLTemplate->addElement("solo",
			std::bind(&SeqTrack::getSolo, this),
			std::bind(&SeqTrack::setSolo, this, _1));
	//classBaseXMLTemplate->addVecElement("event", &eventsRef, std::bind(&SeqTrack::addEvent, this, _1));

	trueStr = "true";
	falseStr = "false";
}

//----------------------------------

SeqElement* SeqTrack::setId(std::string _in)
{
	std::cout << "SeqTrack::setId " << _in << std::endl;
	trackId = _in;
	t_element->set_attribute("id", _in);
	return 0;
}

// ------------------------------------

SeqElement* SeqTrack::setName(std::string _in)
{
	name = _in;
	nameElement->set_child_text(_in);
	return 0;
}

// ------------------------------------

SeqElement* SeqTrack::setMute(std::string _in)
{
	bool val = (bool) std::atoi(_in.c_str());
	mute = val;
	bool resched = !mute && val;

	muteElement->set_child_text(_in);

	//	if ( trackMute.notNil){
//		{ trackMute.value_(val); }.defer;
//	}

//	events.collect({|evt, i|
//		evt.setMute(val);
//	});

// if the track is muted, the scheduled events must be removed,
// unfortunaly this can only be done by clearing the whole queue and rebuilding it
	if (resched)
	{
		//guiPar[\sequencer].reschedule;
	}
	return 0;
}

// ------------------------------------

SeqElement* SeqTrack::setSolo(std::string _in)
{
	bool hasSolo = false;
	bool val = (bool) std::atoi(_in.c_str());

	soloElement->set_child_text(_in);

	/*
	 parentTracks.tracks.collect({|track, i|
	 if(track.solo == true, { hasSolo = true });
	 });

	 parentTracks.tracks.collect({|track, i|
	 if(hasSolo == false, {
	 parentTracks.tracks.collect({|track, i|
	 track.setSoloMute(false);
	 });
	 }, {
	 if(track.solo == true, {
	 track.setSoloMute(false);
	 }, {
	 track.setSoloMute(true);
	 });
	 });
	 });
	 */
	return 0;
}

// ------------------------------------

void SeqTrack::setSoloMute(bool _val)
{
	bool resched = !soloMute && _val;
	soloMute = _val;

//	events.collect({|evt, i|
//		evt.setSoloMute(val);
//	});

	// if the track is muted, the scheduled events must be removed,
	// unfortunaly this can only be done by clearing the whole queue and rebuilding it
	if (resched)
	{
		//guiPar[\sequencer].reschedule;
	}
}

//----------------------------------

std::string* SeqTrack::getName()
{
	return &name;
}

//----------------------------------

std::string* SeqTrack::getMute()
{
	return mute ? &trueStr : &falseStr;
}

//----------------------------------

std::string* SeqTrack::getSolo()
{
	return &name;
}

//----------------------------------

SeqElement* SeqTrack::addEvent(std::string _in)
{
//	events.push_back( SeqEvent.new(ownerDocument, this, this, guiPar, selection, 0, events.size * seqDist), id.asInteger ) );

	return (SeqElement*) events.back();
}

//----------------------------------

SeqTrack::~SeqTrack()
{
}

} /* namespace tav */
