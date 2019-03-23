/*
 * Sequencer.cpp
 *
 *  Created on: 23.02.2017
 *      Copyright by Sven Hahne
 */

#include "Sequencer.h"

namespace tav
{

Sequencer::Sequencer(std::string* _dataPath) :
		dataPath(_dataPath)
{
}

//---------------------------------------------------------------

Sequencer::~Sequencer()
{
}

//---------------------------------------------------------------

void Sequencer::loadXml(const char* _xmlString)
{
	xmlString = _xmlString;

	std::cout
			<< "---------------------------------------------------------------"
			<< std::endl;
	std::cout << "Sequencer::loadXml: " << _xmlString << std::endl;

	// if there was a doc before delete it
	if (doc)
		deleteDoc();

	// create a new DomDocument and the root for the whole sequence structure
	doc = new xmlpp::Document();
//	doc->set_internal_subset("Setup", "", "xample.dtd");
//	doc->set_entity_declaration("xml", xmlpp::XML_INTERNAL_GENERAL_ENTITY, "", "example.dtd", "Extensible Markup Language");
	std::cout << "Sequencer:: created a new doc: " << std::endl;

	// create a root node
	nodeRoot = doc->create_root_node("setup");
	std::cout << "Sequencer:: created a new root called setup: " << std::endl;

	//xmlpp::add_child_entity_reference(nodeRoot, "xml");

#ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
	try
	{
#endif
	std::cout << "Sequencer:: now parsing the input file " << std::endl;

	std::locale::global(std::locale(""));
	setlocale(LC_NUMERIC, "C");

	// convert xmlString to istream
//	    char* c = (char*)_xmlString;
//	    membuf sbuf(c, c + sizeof(c));
//	    std::istream charAsStream(&sbuf);

// read the xml Document from file
	std::string fn(_xmlString);
	xmlpp::DomParser parser((*dataPath) + fn);

	std::cout << "Sequencer:: getting root node " << std::endl;
	xmlpp::Node* pRoot = parser.get_document()->get_root_node(); //deleted by DomParser.
	xmlpp::Node::NodeList tracksNodes = pRoot->get_children("tracks");

	std::cout << "Sequencer:: looking for the next <tracks> Node, found: "
			<< tracksNodes.size() << std::endl;
	if (tracksNodes.size() > 0)
	{
		std::cout
				<< "found tracks, creating a new SeqTracks Object, which will be automatically be added to the rootNode "
				<< std::endl;

		// create a new Tracks Object, which already is a child node of the newly created doc
		tracks = new SeqTracks(nodeRoot, doc);
		xmlpp::Element* nodeElement =
				dynamic_cast<xmlpp::Element*>(tracksNodes.front());
		tracks->loadXml(nodeElement);
	}

#ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
}
catch(const std::exception& ex)
{
	std::cout << "Exception caught: " << ex.what() << std::endl;
}
#endif

	// print result
	Glib::ustring whole = doc->write_to_string();

	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << "XML built at runtime: " << std::endl << whole << std::endl;
}

//---------------------------------------------------------------

void Sequencer::deleteDoc()
{
	delete tracks;
	delete nodeRoot;
	delete doc;
}

} /* namespace tav */
