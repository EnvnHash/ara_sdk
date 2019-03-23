/*
 * SeqEvent.h
 *
 *  Created on: 22.03.2017
 *      Copyright by Sven Hahne
 */

#ifndef SEQUENCER_SEQEVENT_H_
#define SEQUENCER_SEQEVENT_H_

#pragma once

#include "Sequencer/SeqElement.h"

namespace tav
{

class SeqEvent: public SeqElement
{
public:
	SeqEvent(xmlpp::Element* element, xmlpp::Document* _doc);
	virtual ~SeqEvent();
};

} /* namespace tav */

#endif /* SEQUENCER_SEQEVENT_H_ */
