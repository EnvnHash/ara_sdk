/*
 * SeqElement.cpp
 *
 *  Created on: 07.03.2017
 *      Copyright by Sven Hahne
 */

#include "Sequencer/SeqElement.h"
#include "Sequencer/SeqElementTempl.h"

namespace tav
{

SeqElement::SeqElement(xmlpp::Element* element, const char* tagName,
		xmlpp::Document* _doc) :
		parentNode(element), doc(_doc)
{
	std::cout << "call SeqElement init, tagName: " << tagName << std::endl;
	t_element = createNewElement(element, tagName);
}

//----------------------------------

xmlpp::Element* SeqElement::createNewElement(xmlpp::Element* element,
		const char* tagName)
{
	// A good implementation of add_child_entity_reference() ought to check
	// if xmlNewReference() or xmlNewCharRef() shall be called.
	xmlNode* node = xmlNewNode(element->cobj()->ns, (const xmlChar*) tagName);
	node = xmlAddChild(element->cobj(), node);
	xmlpp::Node::create_wrapper(node);
	return static_cast<xmlpp::Element*>(node->_private);
}

//----------------------------------

void SeqElement::setId(unsigned int _inId)
{
	id = _inId;
	t_element->set_attribute("id", std::to_string(_inId));
}
//----------------------------------

unsigned int SeqElement::getId()
{
	return id;
}

//----------------------------------

void SeqElement::procTemplate(SeqElementTempl* inTempl,
		xmlpp::Element* inParentNode, SeqElement* inInst,
		xmlpp::Element* actNode)
{
	if (!templateWasProcessed)
	{
		procTemplateIt(inTempl, inParentNode, inInst, actNode, 0);
		templateWasProcessed = true;
	}
}

//----------------------------------
// this function is called during instancion of the subclasses
// no new instances will be create, only added what is there to build
// a complete xml representation of the SeqElement

void SeqElement::procTemplateIt(SeqElementTempl* inTempl,
		xmlpp::Element* inParentNode, SeqElement* inInst,
		xmlpp::Element* actNode, unsigned int ind)
{
	xmlpp::Element* newParent = inParentNode;

	if (inTempl)
	{
		//("---- PinoGuiSeqElement::procTemplate: "++inTempl.tagName<< std::endl;;

		// set attributes, they come in as [name [String], readFunction, writeFunction ]
		//("PinoGuiSeqElement::procTemplate templ arguments "++inTempl.arguments<< std::endl;;
		for (std::vector<SeqElementTempl::attribObj>::iterator it =
				inTempl->getArguments()->begin();
				it != inTempl->getArguments()->end(); ++it)
		{
			actNode->set_attribute((*it).name, *(*it).getFunc());
		}

		if (inInst)
		{
			// if this is the classBaseXMLTemplate, add this to the parentNode
			if (std::strcmp(inTempl->getName()->c_str(),
					inInst->get_name().c_str()) == 0)
			{
				//("PinoGuiSeqElement::procTemplate got the base Template: "++inTempl.tagName<< std::endl;;
				// hack: the parent of the DOMLElement must be set during super.new
				// at initiation it doesnt make any sense, but here it's updated and can later on
				// be used as usual
				inParentNode->add_child(inInst->getElement(),
						inInst->get_name());
				newParent = inInst->getElement();
				//("PinoGuiSeqElement::procTemplate parentNode is now: "++newParent<< std::endl;;

			}
			else
			{
				std::string* nodeVal = inTempl->getNodeValue();
				//("PinoGuiSeqElement::procTemplate readValue: "++nodeVal++" isArray: "++nodeVal.isArray << std::endl;;
				/*
				 if ( (nodeVal.isString == false) && nodeVal.isArray)
				 {
				 // process array elements
				 nodeVal.collect({|item, i|
				 //("PinoGuiSeqElement::procTemplate process array element ["++i++"] "++item<< std::endl;;
				 if (item.class.superclass.asString == "PinoGuiSeqElement")
				 {
				 procTemplateIt( item.classBaseXMLTemplate, newParent, item, item );
				 } else {
				 procTemplateIt( item, newParent, inInst, nodeVal );
				 }
				 }
				 } else
				 {
				 */
				if (nodeVal)
				{
					//("PinoGuiSeqElement::procTemplate add straight as a child and take as new parent: "<< std::endl;;
					//("PinoGuiSeqElement::procTemplate inParentNode: "++inParentNode++" nodeVAl: "++nodeVal<< std::endl;;
					// straight content, (string, number, boolean, etc.)
					xmlpp::Element* el = inParentNode->add_child(
							*inTempl->getName());
					el->add_child_text(*nodeVal);
					newParent = el;
				}
				//}
			}
		}

		//("PinoGuiSeqElement::procTemplate subelements "++inTempl.subElements<< std::endl;;
		// process subelements
		unsigned int ind = 0;
		for (std::vector<SeqElementTempl>::iterator it =
				inTempl->getSubElements()->begin();
				it != inTempl->getSubElements()->end(); ++it)
		{
			//var nodeVal = item.nodeValue.value(i);
			xmlpp::Element* el = newParent->add_child(*(*it).getName());

			//("PinoGuiSeqElement::procTemplate proc subelements nodeVal: "++nodeVal.class<< std::endl;;
			//("PinoGuiSeqElement::procTemplate reiterate with: "++[ item, newParent, inInst, nodeVal, i]<< std::endl;;
			procTemplateIt(&(*it), newParent, inInst, el, ind);
			ind++;
		}
	}
}

//----------------------------------

void SeqElement::loadXmlIt(xmlpp::Element* node, SeqElementTempl* actTempl,
		SeqElement* inst, unsigned int ind)
{
	std::cout << "PinoGuiSeqElement::loadXmlIt, classTemplate.tagName: "
			<< *actTempl->getName() << " node: " << std::endl;

	std::string whole = doc->write_to_string();
	std::cout << whole << std::endl;

	// read attributes, [name [String], readFunction, writeFunction ]
	std::cout << "PinoGuiSeqElement::loadXmlIt, actual templ: "
			<< *actTempl->getName() << " arguments: "
			<< actTempl->getArguments()->size() << std::endl;

	for (std::vector<SeqElementTempl::attribObj>::iterator it =
			actTempl->getArguments()->begin();
			it != actTempl->getArguments()->end(); ++it)
	{
		std::cout << "PinoGuiSeqElement::loadXmlIt, set argument: "
				<< (*it).name << " = " << node->get_attribute_value((*it).name)
				<< std::endl;
		(*it).setFunc(node->get_attribute_value((*it).name));
	}

	// check if there is a Child TextNode
	xmlpp::Node::NodeList children = node->get_children();
	for (xmlpp::Node::NodeList::iterator it = children.begin();
			it != children.end(); ++it)
	{
		xmlpp::Element* el = static_cast<xmlpp::Element*>(*it);
		//if (item.getNodeType == 3)
		//("PinoGuiSeqElement::loadXmlIt node has text: "++item.getText<< std::endl;;
		//actTempl.loadFunc( el->get_child_text() );
		//actTempl->setNodeValue( el->get_child_text() );
	}

	// process subelements
	std::cout << "PinoGuiSeqElement::loadXmlIt subelements of actTempl "
			<< *actTempl->getName() << ": "
			<< actTempl->getSubElements()->size() << std::endl;

	for (std::vector<SeqElementTempl>::iterator it =
			actTempl->getSubElements()->begin();
			it != actTempl->getSubElements()->end(); ++it)
	{
		bool checkIds = true;
		bool found = false;
		xmlpp::Node::NodeList children = node->get_children();// children of actual node

		std::cout << "PinoGuiSeqElement::loadXmlIt subelement ["
				<< it - actTempl->getSubElements()->begin();
		std::cout << "] tagName: " << *(*it).getName() << std::endl;

		// is this a single value or a vector with dynamical size?
		if (!(*it).isDynSize)
		{
			std::cout
					<< "PinoGuiSeqElement::loadXmlIt subelement is a single Value"
					<< std::endl;
			checkIds = false;
		}
		else
		{
			std::cout << "PinoGuiSeqElement::loadXmlIt subelement is a vector"
					<< std::endl;
		}

		// get SubElement from node
		if (checkIds)
		{
			unsigned int ind = 0;
			xmlpp::Node::NodeList::iterator thisChild = children.begin();

			// iterate through childNodes and search for the name of the actual template
			while (found == false && ind < children.size())
			{
				if (std::strcmp((*thisChild)->get_name().c_str(), "text") != 0
						&& (std::strcmp((*thisChild)->get_name().c_str(),
								(*it).getName()->c_str()) == 0))
				{
					unsigned int nvInd = 0;
					bool foundNv = false;
					std::string childText = "";
					SeqElement* itInst = inst;
					xmlpp::Element* itAsElem =
							static_cast<xmlpp::Element*>(*thisChild);
					std::string id = "-1";
					std::string zero = "0";

					if (itAsElem->get_attribute("id") != 0)
						id = itAsElem->get_attribute("id")->get_value().c_str();

					std::cout
							<< "PinoGuiSeqElement::loadXmlIt proc SubELements: ind: "
							<< ind << " children[" << ind << "] = "
							<< (*thisChild)->get_name();
					std::cout << " got attrib id: " << id << std::endl;

					if (itAsElem->has_child_text())
					{
						std::cout << " has has_child_text: "
								<< itAsElem->get_child_text()->get_content().size()
								<< std::endl;
						childText = itAsElem->get_child_text()->get_content();
					}

					std::cout << "PinoGuiSeqElement::loadXmlIt found "
							<< (*thisChild)->get_name() << " with id: " << id
							<< std::endl;
					found = true;

					// check if the object already exists or has to be created
					//("PinoGuiSeqElement::loadXmlIt search the nodeValues "++nodeVal<< std::endl;;
					if (std::atoi(id.c_str()) != -1
							&& (*it).getNodeVector()->size() > 0)
					{
						std::vector<SeqElement*>* nodeVec =
								(*it).getNodeVector();
						while (!foundNv && nvInd < nodeVec->size())
						{
							if (nodeVec->at(nvInd)->getId()
									== std::atoi(id.c_str()))
							{
								foundNv = true;
								// instance exist, iterate into it
								itInst = nodeVec->at(nvInd);
								std::cout
										<< "PinoGuiSeqElement::loadXmlIt found the id in the nodeValues, new Inst: "
										<< itInst << std::endl;
							}
							nvInd = nvInd + 1;
						}
					}

					// if id wasn't found, create a new instance
					if (!foundNv)
					{
						std::cout
								<< "PinoGuiSeqElement::loadXmlIt id wasn't found, create new instance"
								<< std::endl;
						if (std::atoi(id.c_str()) != -1)
						{
							itInst = (*it).setFunc((std::string) id);
						}
						else
						{
							itInst = (*it).setFunc((std::string) zero);
						}

						std::cout
								<< "PinoGuiSeqElement::loadXmlIt created new Inst: "
								<< itInst << std::endl;
					}

					if (itInst)
					{
						std::cout
								<< "PinoGuiSeqElement::loadXmlIt reiterate node: "
								<< (*thisChild)->get_name();
						std::cout << " actTempl: "
								<< itInst->classBaseXMLTemplate << " itInst: "
								<< itInst << " id:" << id << std::endl;

						// permit another while loop;
						found = false;
						loadXmlIt(itAsElem, itInst->classBaseXMLTemplate,
								itInst, std::atoi(id.c_str()));

						std::cout
								<< "PinoGuiSeqElement::loadXmlIt finished subiterate, call Load Function"
								<< std::endl;

						// call the load func
						if (itInst->classBaseXMLTemplate->setFunc != 0)
							itInst->classBaseXMLTemplate->setFunc(
									(std::string) childText);
					}
				}

				thisChild++;
				ind++;
			}
		}
		else
		{

			xmlpp::Node* thisChild = node->get_first_child(*(*it).getName());
			xmlpp::Element* childAsElem =
					static_cast<xmlpp::Element*>(thisChild);

			if (childAsElem)
			{
				std::cout << "PinoGuiSeqElement::loadXmlIt found "
						<< *(*it).getName() << " get Element: "
						<< childAsElem->get_name() << std::endl;
				std::cout << "PinoGuiSeqElement::loadXmlIt nodeVal "
						<< childAsElem->get_child_text()->get_content()
						<< std::endl;

				// if the node value is a PinoGuiSeqElement, reiterate
				//if ( nodeVal.class.superclass.asString == "PinoGuiSeqElement")
				if ((*it).isDynSize)
				{
					//("PinoGuiSeqElement::loadXmlIt nodeVal is PinoGuiSeqElement, reiterate with values: "++[getSubNode, item, nodeVal, 0 ]<< std::endl;;

					//this.loadXmlIt( getSubNode, nodeVal.classBaseXMLTemplate, nodeVal, 0 );
					//nodeVal.classBaseXMLTemplate.loadFunc.value( getSubNode );
				}
				else
				{
					(*it).setFunc(childAsElem->get_child_text()->get_content());
				}
			}
		}
	}
}

//----------------------------------

void SeqElement::loadXml(xmlpp::Element* node)
{
	if (classBaseXMLTemplate)
		loadXmlIt(node, classBaseXMLTemplate, this, 0);
}

//----------------------------------

const std::string SeqElement::get_name()
{
	return t_element->get_name();
}

//----------------------------------

void SeqElement::set_name(std::string _name)
{
	t_element->set_name(_name);
}

//----------------------------------

xmlpp::Element* SeqElement::getElement()
{
	return t_element;
}

//----------------------------------

SeqElement::~SeqElement()
{
}

} /* namespace tav */
