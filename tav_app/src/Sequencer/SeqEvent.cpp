/*
 * SeqEvent.cpp
 *
 *  Created on: 22.03.2017
 *      Copyright by Sven Hahne
 */

#include <Sequencer/SeqEvent.h>

namespace tav
{

SeqEvent::SeqEvent(xmlpp::Element* element, xmlpp::Document* _doc) :
		SeqElement(element, "event", _doc)
{

}

//---------------------------------------------------------------

SeqEvent::~SeqEvent()
{
	// TODO Auto-generated destructor stub
}

} /* namespace tav */
