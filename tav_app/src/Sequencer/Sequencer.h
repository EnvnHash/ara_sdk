/*
 * Sequencer.h
 *
 *  Created on: 23.02.2017
 *      Copyright by Sven Hahne
 */

#ifndef SEQUENCER_SEQUENCER_H_
#define SEQUENCER_SEQUENCER_H_

#pragma once

#include <iostream>
#include <istream>
#include <streambuf>
#include <string>
#include <libxml++/libxml++.h>

#include "SeqTracks.h"
#include "TimedFunction.h"

namespace tav
{

class Sequencer
{
public:
	// helper for converting const char* to istream
//	struct membuf : std::streambuf
//	{
//	    membuf(char* begin, char* end) {
//	        this->setg(begin, begin, end);
//	    }
//	};

	Sequencer(std::string* _dataPath);
	virtual ~Sequencer();
	void loadXml(const char* _xmlString);
	void deleteDoc();
private:
	const char* xmlString;

	//xmlpp::DomParser 	parser;
	xmlpp::Document* doc = 0;
	xmlpp::Element* nodeRoot = 0;

	SeqTracks* tracks = 0;

	std::string* dataPath;
};

} /* namespace tav */

#endif /* SEQUENCER_SEQUENCER_H_ */
