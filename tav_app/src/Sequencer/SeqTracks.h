/*
 * SeqTracks.h
 *
 *  Created on: 22.03.2017
 *      Copyright by Sven Hahne
 */

#ifndef SEQUENCER_SEQTRACKS_H_
#define SEQUENCER_SEQTRACKS_H_

#pragma once

#include <iostream>

#include "Sequencer/SeqEvent.h"
#include "Sequencer/SeqElementTempl.h"

namespace tav
{

class SeqTrack;
// forwards declaration

class SeqTracks: public SeqElement
{
public:
	SeqTracks(xmlpp::Element* element, xmlpp::Document* _doc);
	SeqElement* addTrack(std::string _str);
	virtual ~SeqTracks();
private:
	std::vector<SeqTrack*> tracks;
	std::vector<SeqElement*> tracksRef;

	SeqEvent* parentMetaEvent;
	int trackIdCntr = 0;
};

} /* namespace tav */

#endif /* SEQUENCER_SEQTRACKS_H_ */
