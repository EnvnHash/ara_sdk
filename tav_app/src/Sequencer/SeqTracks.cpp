/*
 * SeqTracks.cpp
 *
 *  Created on: 22.03.2017
 *      Copyright by Sven Hahne
 */

#include "Sequencer/SeqTracks.h"
#include "Sequencer/SeqTrack.h"

namespace tav
{

SeqTracks::SeqTracks(xmlpp::Element* element, xmlpp::Document* _doc) :
		SeqElement(element, "tracks", _doc)
{
	classBaseXMLTemplate = new SeqElementTempl(get_name().c_str());

	using std::placeholders::_1;
	classBaseXMLTemplate->addVecElement("track", &tracksRef,
			std::bind(&SeqTracks::addTrack, this, _1));
}

//----------------------------------

SeqElement* SeqTracks::addTrack(std::string _id)
{
	unsigned int id = 0;

	std::cout << "SeqTracks::addTrack" << std::endl;

	tracks.push_back(
			new SeqTrack(t_element, doc, tracks.size(), parentMetaEvent, this));

	if (std::strcmp(_id.c_str(), "") != 0)
	{
		id = std::atoi(_id.c_str());

		tracks.back()->setId(_id);
		if (id > trackIdCntr)
			trackIdCntr = id;

	}
	else
	{
		std::string idStr = std::to_string(trackIdCntr);
		tracks.back()->setId(idStr);
	}

	trackIdCntr++;

	//t_element->add_child(tracks.back()->get_name(), tracks.back()->getElement());

	// return the new object
	return tracks.back();
}

//----------------------------------

SeqTracks::~SeqTracks()
{
}

} /* namespace tav */
