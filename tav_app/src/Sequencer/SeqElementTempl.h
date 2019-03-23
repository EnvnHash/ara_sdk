/*
 * SeqElementTempl.h
 *
 *  Created on: 07.03.2017
 *      Copyright by Sven Hahne
 */

#ifndef SEQUENCER_SEQELEMENTTEMPL_H_
#define SEQUENCER_SEQELEMENTTEMPL_H_

#include <functional>
#include <cstring>
#include <vector>
#include <iostream>
#include <libxml++/libxml++.h>

namespace tav
{

class SeqElement;
// forward declaration

class SeqElementTempl
{
public:

	typedef struct
	{
		std::string name;
		std::function<std::string*()> getFunc;
		std::function<SeqElement*(std::string _str)> setFunc;
	} attribObj;

	SeqElementTempl(const char* _tagName,
			std::function<SeqElement*(std::string _str)> _setFunc = 0);
	virtual ~SeqElementTempl();

	xmlpp::Element* createIt(xmlpp::Document* doc, xmlpp::Element* root,
			SeqElementTempl* elem);
	xmlpp::Element* create(xmlpp::Document* doc, xmlpp::Element* root);

	void update(xmlpp::Document* doc, xmlpp::Element* root);
	void updateIt(xmlpp::Document* doc, xmlpp::Element* root,
			SeqElementTempl* elem, unsigned int ind);

	void addArg(std::string _name, std::function<std::string*()> _getFunc,
			std::function<SeqElement*(std::string _str)> _setFunc);
	void addArg(attribObj _args);
	bool hasArg(std::string _argName);
	void clearArgs();

	// add a single subnode
	SeqElementTempl* addElement(const char* _tagName,
			std::function<std::string*()> _getFunc,
			std::function<SeqElement*(std::string _str)> _setFunc);

	// add a vector of subnodes, dynamic size
	SeqElementTempl* addVecElement(const char* _tagName,
			std::vector<SeqElement*>* _nodeVector,
			std::function<SeqElement*(std::string _str)> _setFunc);

	// add a list of nodes, elements will be added individually
	std::vector<SeqElementTempl>::const_reference addElementList(
			const char* tagName,
			std::vector<std::function<std::string*()>*>* _getFuncAr,
			std::vector<attribObj> argList);
	//void updateElement(std::string tagName, std::string* _nodeValue);
	void removeElement(std::string tagName);

	std::string* getName();
	std::string* getNodeValue();
	std::vector<SeqElement*>* getNodeVector();

	std::vector<SeqElementTempl::attribObj>* getArguments();
	std::vector<SeqElementTempl>* getSubElements();
	SeqElementTempl* getElement(std::string _tagName);
	std::string* getPrototype();
	std::vector<SeqElementTempl> getAllElements(std::string tagName);
	void clearElementList();

	void setNode(xmlpp::Node* _node);
	void setNodeValueFunc(std::function<std::string*()> _getFunc);
	void setNodeVector(std::vector<SeqElement*>* _nodeVector);

	std::function<SeqElement*(std::string _str)> setFunc;
	bool isDynSize = false;

private:
	std::string tagName = "";
	std::vector<attribObj> arguments;// must be [name [String], readFunction, writeFunction ]
	std::vector<SeqElementTempl> subElements;

	xmlpp::Node* node = 0;

	std::function<std::string*()> nodeValueFunc;
	std::vector<SeqElement*>* nodeVector = 0;

	std::string prototype;
};

} /* namespace tav */

#endif /* SEQUENCER_SEQELEMENTTEMPL_H_ */
