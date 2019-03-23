/*
 * SeqElement.h
 *
 *  Created on: 07.03.2017
 *      Copyright by Sven Hahne
 */

#ifndef SEQUENCER_SEQELEMENT_H_
#define SEQUENCER_SEQELEMENT_H_

#pragma once

#include <libxml++/libxml++.h>
#include <libxml/tree.h>
#include <cstring>

namespace tav
{

class SeqElementTempl;
// forward declaration

class SeqElement
{
public:
	SeqElement(xmlpp::Element* element, const char* tagName,
			xmlpp::Document* _doc);
	xmlpp::Element* createNewElement(xmlpp::Element* element,
			const char* tagName);
	void setId(unsigned int _inId);
	unsigned int getId();
	void procTemplate(SeqElementTempl* inTempl, xmlpp::Element* inParentNode,
			SeqElement* inInst, xmlpp::Element* actNode);
	void procTemplateIt(SeqElementTempl* inTempl, xmlpp::Element* inParentNode,
			SeqElement* inInst, xmlpp::Element* actNode, unsigned int ind);
	void loadXmlIt(xmlpp::Element* node, SeqElementTempl* actTempl,
			SeqElement* inst, unsigned int ind);
	void loadXml(xmlpp::Element* node);

	// xmlpp::Element funcitonality
	void set_name(std::string _name);
	const std::string get_name();
	xmlpp::Element* getElement();

	virtual ~SeqElement();

	SeqElementTempl* classBaseXMLTemplate;
	unsigned int id;
	xmlpp::Element* t_element;
	xmlpp::Document* doc;

private:

	xmlpp::Node* parentNode;
	xmlpp::Document* lastDomDoc;
	xmlpp::Node* lastRoot;
	bool templateWasProcessed = false;

	//lastXmlDoc;
};

} /* namespace tav */

#endif /* SEQUENCER_SEQELEMENT_H_ */
