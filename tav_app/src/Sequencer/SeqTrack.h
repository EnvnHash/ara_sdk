/*
 * SeqTrack.h
 *
 *  Created on: 22.03.2017
 *      Copyright by Sven Hahne
 */

#ifndef SEQUENCER_SEQTRACK_H_
#define SEQUENCER_SEQTRACK_H_

#pragma once

#include "Sequencer/SeqElement.h"
#include "Sequencer/SeqEvent.h"

namespace tav
{
class SeqTracks;
// forwards declaration

class SeqTrack: public SeqElement
{
public:
	SeqTrack(xmlpp::Element* element, xmlpp::Document* _doc, unsigned int _id,
			SeqEvent* _metaEvent, SeqTracks* _parentTracks);
	virtual ~SeqTrack();

	SeqElement* setId(std::string _in);
	SeqElement* setName(std::string _in);
	SeqElement* setMute(std::string _in);
	SeqElement* setSolo(std::string _in);

	void setSoloMute(bool _val);

	SeqElement* addEvent(std::string _in);

	std::string* getName();
	std::string* getMute();
	std::string* getSolo();

private:
	SeqTracks* parentTracks;
	SeqEvent* metaEvent;
	std::vector<SeqEvent*> events;
	std::vector<SeqElement*> eventsRef;
	std::string trackId;
	std::string name;

	xmlpp::Element* nameElement;
	xmlpp::Element* muteElement;
	xmlpp::Element* soloElement;

	bool soloMute = false;
	bool mute = false;

	std::string trueStr;
	std::string falseStr;

};

} /* namespace tav */

#endif /* SEQUENCER_SEQTRACK_H_ */
