/*
 * SeqElementTempl.cpp
 *
 *  Created on: 07.03.2017
 *      Copyright by Sven Hahne
 */

#include <Sequencer/SeqElementTempl.h>
#include "SeqElement.h"

namespace tav
{

SeqElementTempl::SeqElementTempl(const char* _tagName,
		std::function<SeqElement*(std::string _str)> _setFunc) :
		tagName(_tagName), setFunc(_setFunc)
{
}

//----------------------------------

xmlpp::Element* SeqElementTempl::createIt(xmlpp::Document* doc,
		xmlpp::Element* root, SeqElementTempl* elem)
{
	unsigned int hasContent = false;
	std::string* elemNodeValue = elem->getNodeValue();
	xmlpp::Element* newDomElem = root->add_child(*elem->getName());

	std::string xmlString = doc->write_to_string();
	std::cerr << "SeqElementTempl::creating newDomElem: " << xmlString
			<< std::endl;
	std::cerr << "SeqElementTempl::creating tag: " << *elem->getName()
			<< std::endl;

	// add Arguments
	std::cout << "SeqElementTempl::createIt arguments " << elem->getArguments()
			<< std::endl;
	;

	for (std::vector<attribObj>::iterator it = elem->getArguments()->begin();
			it != elem->getArguments()->end(); ++it)
	{
		std::cout << "SeqElementTempl::createIt arguments " << (*it).name
				<< " attrVal: " << *(*it).getFunc() << std::endl;
		newDomElem->set_attribute((*it).name, *(*it).getFunc());
		hasContent = true;
	}

	// add nodeValue
	if (std::strcmp(elemNodeValue->c_str(), "") != 0)
	{
		std::cout << "SeqElementTempl::createIt elemNodeValue: "
				<< (*elemNodeValue) << std::endl;

		newDomElem->set_child_text(*elemNodeValue);
		elem->setNode(newDomElem);
		hasContent = true;
	}

	std::cerr << "SeqElementTempl::creating elem.subElements: "
			<< elem->getSubElements() << std::endl;

	// add SubElements
	for (std::vector<SeqElementTempl>::iterator it = subElements.begin();
			it != subElements.end(); ++it)
	{
		std::cout << "SeqElementTempl::createIt subElements " << std::endl;

		xmlpp::Element* subNode = createIt(doc, root, &(*it));

		if (subNode)
		{
			//newDomElem->a>add_child(subNode);
			hasContent = true;
		}
	}

	std::cout << "SeqElementTempl::createIt finished " << std::endl;

	if (hasContent == false)
	{
		return 0;
	}
	else
	{
		xmlString = doc->write_to_string();
		std::cout << xmlString << std::endl;
		return newDomElem;
	}
}

//----------------------------------

xmlpp::Element* SeqElementTempl::create(xmlpp::Document* doc,
		xmlpp::Element* root)
{
	xmlpp::Element* newElem = createIt(doc, root, this);
	return newElem;
}

//----------------------------------
// update all XMLElements of in this Node-level, don't go deeper
// if there is a new Element that has to be created, call it's creator.
// root = classXmlNode of PinoGuiSeqElement
void SeqElementTempl::updateIt(xmlpp::Document* doc, xmlpp::Element* root,
		SeqElementTempl* elem, unsigned int ind)
{
	xmlpp::Node* childNode;

	std::cout << "------------SeqElementTempl::update, elem.getName(): "
			<< elem->getName() << " ind: " << ind << " elem.nodeValue: "
			<< elem->getNodeValue() << std::endl;

	// set Element NodeValue
	if (elem->getNodeValue())
	{
		xmlpp::Node::NodeList childNodes = root->get_children();

		if (childNodes.size() > 0)
			childNode = &childNode[0];

		std::cout << "SeqElementTempl::update nodeValue: "
				<< elem->getNodeValue() << " ind: " << ind << std::endl;

		if (std::strcmp((*elem->getNodeValue()).c_str(), "") != 0)
		{
			std::cout << "SeqElementTempl::update nodeValue is : "
					<< *elem->getNodeValue() << std::endl;
			;

			//if(childNode.class.asString == "DOMText") {
			childNode->get_parent()->add_child_text(*elem->getNodeValue());
			//} else {
			//	root->add_child_text( *elem->getNodeValue() );
			//}
		}
		else
		{
			//if(childNode.class.asString == "DOMText") {
			childNode->get_parent()->add_child_text(*elem->getNodeValue());
			//} else {
			//	root->add_child_text( *elem->getNodeValue() );
			//}
		}
	}

	/*
	 //std::cout << "SeqElementTempl::update, elem.arguments: " << elem.arguments << std::endl;;

	 // update attributes
	 elem.arguments.collect({|pair, i|
	 //std::cout << "SeqElementTempl::update attributes " << pair << std::endl;;
	 //std::cout << "SeqElementTempl::update attributes " << pair[0] << ": " << pair[1] << " value[" << ind << "]: " << pair[1].value(ind) << std::endl;;
	 root.set_attribute(pair[0], pair[1].value(ind));
	 });

	 std::cout << "SeqElementTempl::update subElements: " << elem.subElements << std::endl;;

	 // update subelements
	 // also adjust number of subelments, create or destroy them
	 elem.subElements.collect({|item, i|
	 unsigned int flatItem = this.checkElem(item);
	 unsigned int seqElemInst = this.isSeqElement(item);
	 unsigned int thisNode = flatItem.node;

	 std::cout << "SeqElementTempl::update subElements, item: " << flatItem << " tagName: " << flatItem.tagName << " has node: " << flatItem.node << std::endl;;

	 // if the subElem has no XMLNode associated to it, create it
	 if (thisNode.notNil == false) {
	 unsigned int newNode;

	 std::cout << "SeqElementTempl::update create node, seqElemInst: " << seqElemInst << std::endl;;
	 std::cout << "SeqElementTempl::update flatItem.nodeValue: " << flatItem.nodeValue << std::endl;;
	 if ( flatItem.nodeValue.class.asString == "Function") {
	 std::cout << "SeqElementTempl::update flatItem.nodeValue is Function, value: " << flatItem.nodeValue.value << std::endl;;
	 });

	 if (this.isSimpleunsigned int(flatItem.nodeValue.value) == false) {

	 std::cout << "SeqElementTempl::update no simple unsigned int create: " << flatItem.tagName << std::endl;;

	 std::cout << "SeqElementTempl::checkElem is a PinoGuiSeqElement, classBaseXMLElement:" << flatItem.classBaseXMLElement  << std::endl;;

	 newNode = flatItem.create(doc, root, flatItem);

	 std::cout << "SeqElementTempl::update create ready, newNode: " << newNode << std::endl;;
	 if ( newNode.notNil) { newNode.format(0 << std::endl;; });

	 if (newNode.notNil && seqElemInst.notNil) {
	 // if this was an Instance of a PinoGuiSeqElement and not a SeqElementTempl set the classXmlNode
	 //std::cout << ">>>> SeqElementTempl::update  newNode: " << newNode << std::endl;;
	 seqElemInst.classXmlNode = newNode;
	 });
	 } else {
	 newNode = doc.createElement(flatItem.tagName);
	 newNode.createTextNode(flatItem.nodeValue.value);
	 });

	 if (newNode.notNil) {
	 root.appendChild(newNode);
	 thisNode = newNode;
	 });
	 });

	 // update the values
	 if ( thisNode.notNil) {
	 std::cout << "SeqElementTempl::update update values of subelement, node: " << std::endl;;
	 thisNode.format(0 << std::endl;;

	 this.updateIt(doc, thisNode, flatItem, i);
	 });
	 });
	 */
}

//----------------------------------

void SeqElementTempl::update(xmlpp::Document* doc, xmlpp::Element* root)
{
	updateIt(doc, root, this, 0);
}

//----------------------------------
// must be [name [String], readFunction, writeFunction ]
// readFunction -> Wert der gesetzt wird wenn das Attribute geschrieben wird
// writeFunction -> Function die ausgefuehrt wird, wenn das Attribute aus einem XMLString gelesen wird
// 	und eine Klassen-Variable geschrieben werden soll
void SeqElementTempl::addArg(std::string _name,
		std::function<std::string*()> _getFunc,
		std::function<SeqElement*(std::string _str)> _setFunc)
{
	arguments.push_back(attribObj());
	arguments.back().name = _name;
	arguments.back().getFunc = _getFunc;
	arguments.back().setFunc = _setFunc;
}

//----------------------------------

void SeqElementTempl::addArg(attribObj _args)
{
	arguments.push_back(_args);
}

//----------------------------------
// values must be functions!!! ...problem with references in SC
bool SeqElementTempl::hasArg(std::string _argName)
{
	bool out = false;
	std::vector<attribObj>::iterator it = arguments.begin();

	while (it != arguments.end() && !out)
	{
		if (std::strcmp(_argName.c_str(), (*it).name.c_str()) == 0)
			out = true;
		++it;
	}

	return false;
}

//----------------------------------

void SeqElementTempl::clearArgs()
{
	arguments.clear();
}

//----------------------------------
// nodeValue type[ ]
// loadFunction wird ausgefuehrt, wenn ein neues Element diesen Typs angelegt wird.
// function that return strings or arrays with PinoGuiSeqElements
// Elements are always saved individually, no arrays!!!

SeqElementTempl* SeqElementTempl::addElement(const char* _tagName,
		std::function<std::string*()> _getFunc,
		std::function<SeqElement*(std::string _str)> _setFunc)
{
	subElements.push_back(SeqElementTempl(_tagName, _setFunc));
	subElements.back().setNodeValueFunc(_getFunc);
	return &subElements.back();
}

//----------------------------------

SeqElementTempl* SeqElementTempl::addVecElement(const char* _tagName,
		std::vector<SeqElement*>* _nodeVector,
		std::function<SeqElement*(std::string _str)> _setFunc)
{
	subElements.push_back(SeqElementTempl(_tagName, _setFunc));
	subElements.back().setNodeVector(_nodeVector);
	subElements.back().isDynSize = true;
	return &subElements.back();
}

//----------------------------------
/*
 void SeqElementTempl::updateElement(std::string tagName, std::string* _nodeValue)
 {
 SeqElementTempl* elem = getElement(tagName);
 if (elem)
 {
 elem->setNodeValue( _nodeValue );
 } else
 {
 std::cerr << "SeqElementTempl::updateElement Error, element with tagname not found" << std::endl;
 }
 }
 */
//----------------------------------
void SeqElementTempl::removeElement(std::string tagName)
{
	unsigned int ind = 0;
	bool found = false;

	while ((ind < subElements.size()) && !found)
	{
		if (std::strcmp((*subElements[ind].getName()).c_str(), tagName.c_str())
				== 0)
		{
			found = true;
			subElements.erase(subElements.begin() + ind);
		}
		ind = ind + 1;
	}
}

//----------------------------------

std::string* SeqElementTempl::getName()
{
	return &tagName;
}

//----------------------------------

std::string* SeqElementTempl::getNodeValue()
{
	return nodeValueFunc();
}

//----------------------------------

std::vector<SeqElement*>* SeqElementTempl::getNodeVector()
{
	return nodeVector;
}

//----------------------------------

std::vector<SeqElementTempl::attribObj>* SeqElementTempl::getArguments()
{
	return &arguments;
}

//----------------------------------

std::vector<SeqElementTempl>* SeqElementTempl::getSubElements()
{
	return &subElements;
}

//----------------------------------

SeqElementTempl* SeqElementTempl::getElement(std::string _tagName)
{
	SeqElementTempl* out;
	unsigned int ind = 0;
	bool found = false;

	while ((ind < subElements.size()) && !found)
	{
		if (std::strcmp(subElements[ind].getName()->c_str(), tagName.c_str())
				== 0)
		{
			found = true;
			out = &subElements[ind];
		}

		ind++;
	}

	return out;
}

//----------------------------------

std::string* SeqElementTempl::getPrototype()
{
	return &prototype;
}

//----------------------------------

void SeqElementTempl::setNode(xmlpp::Node* _node)
{
	node = _node;
}

//----------------------------------

void SeqElementTempl::setNodeValueFunc(std::function<std::string*()> _getFunc)
{
	nodeValueFunc = _getFunc;
}

//----------------------------------

void SeqElementTempl::setNodeVector(std::vector<SeqElement*>* _nodeVector)
{
	nodeVector = _nodeVector;
}

//----------------------------------

std::vector<SeqElementTempl> SeqElementTempl::getAllElements(
		std::string tagName)
{
	std::vector<SeqElementTempl> ar;
	for (auto i = 0; i < subElements.size(); i++)
		if (std::strcmp((*subElements[i].getName()).c_str(), tagName.c_str())
				== 0)
			ar.push_back(subElements[i]);

	return ar;
}

//----------------------------------

//----------------------------------
// variable size of elements all with the same tagName,
// optionally there can be a list of arguments in the format [arg, val, arg, val]
// which must be same size as the nodeValueAr
std::vector<SeqElementTempl>::const_reference SeqElementTempl::addElementList(
		const char* tagName,
		std::vector<std::function<std::string*()>*>* _getFuncAr,
		std::vector<attribObj> argList)
{
	for (std::vector<std::function<std::string*()>*>::iterator it =
			_getFuncAr->begin(); it != _getFuncAr->end(); ++it)
	{
		subElements.push_back(SeqElementTempl(tagName));
		subElements.back().setNodeValueFunc(*(*it));

		if (argList.size() > 0)
			subElements.back().addArg(argList[it - _getFuncAr->begin()]);
	}

	return subElements.back();
}

//----------------------------------

void SeqElementTempl::clearElementList()
{
	subElements.clear();
}

//----------------------------------

SeqElementTempl::~SeqElementTempl()
{
	arguments.clear();
	subElements.clear();
}

/*
 //----------------------------------
 // values must be functions!!! ...problem with references in SC
 setArg(argName, val|
 if(arguments.flop[0].indexOf(argName).notNil) {
 arguments[arguments.flop[0].indexOf(argName)][1] = val;
 } else {
 "SeqElementTempl::setArg ArgumentName not found".postln;
 });
 }
 */
} /* namespace tav */
