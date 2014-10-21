#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <boost/program_options.hpp>
#include <libxml++/libxml++.h>
#include <tidy.h>
#include <buffio.h>
#include <glib-object.h>

#include "builder.hh"
#include "html.hh"
#include "latex.hh"
#include "symmap.hh"

namespace po = boost::program_options;
using namespace std;
using namespace xmlpp;

namespace xml2epub {
  unsigned int count_total_xml_elements( const Node & in_node ) {
    unsigned int retval = 1;
    const Element * elem_pre = dynamic_cast<const Element*>(&in_node);
    if ( elem_pre != NULL ) {
      const Element & elem = *elem_pre;
      Node::NodeList list = elem.get_children();
      for ( Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter ) {
	if ( *iter != NULL ) {
	  retval += count_total_xml_elements( **iter );
	}
      }
    }
    return retval;
  }

  void parse_cmdline_args( int argc, char * argv[], bool keep_text, 
			   string & input_file,
			   bool & input_file_is_cin,
			   string & output_file,
			   bool & output_file_is_cout,
			   bool & output_html ) {
    /* defaults */
    keep_text = false;
    input_file = "";
    input_file_is_cin = true;
    output_file = "";
    output_file_is_cout = true;
    output_html = true;
    
    po::options_description desc("Allowed options");
    desc.add_options()
      ( "help,h", "produce help message" )
      ( "keep-text,t", po::value<bool>(), "When converting equations to svg images, keep text or convert to path" )
      ( "input-file,i", po::value< vector<string> >(), "input xml file path (default is standard input)" )
      ( "output-file,o", po::value< vector<string> >(), "output html file" )
      ( "latex,l", po::value<bool>(), "output latex file" );
    po::variables_map vm;
    po::store( po::parse_command_line( argc, argv, desc ), vm );

    if ( vm.count("help") ) {
      cout << desc << endl;
    }
    if ( vm.count("keep-text") ) {
      keep_text = vm["keep-text"].as<bool>();
    }
    if ( vm.count("latex") ) {
      output_html = ( vm["latex"].as<bool>() == false );
    }
    if ( vm.count("input-file") > 1 ) {
      throw runtime_error( "You may only specify one input file (or none for standard input)" );
    }
    if ( vm.count("input-file") == 1 ) {
      input_file = vm["input-file"].as< vector<string> >()[0];
      input_file_is_cin = false;
    }

    if ( vm.count("output-file") > 1 ) {
      throw runtime_error( "You may only specify one output file (or none for standard output)" );
    }
    if ( vm.count("output-file") != 1 ) {
      throw runtime_error( "You must specify an output path" );
    }
    output_file = vm["output-file"].as< vector<string> >()[0];
    output_file_is_cout = false;
  }

  class NodeParser {
  private:
    unsigned int m_total, m_current, m_last_percent;
  public:
    NodeParser( unsigned int total_elements )
      : m_total(total_elements), m_current(0), m_last_percent(0) {
      std::cerr << "0% finished" << std::endl;
    }

    void parse_node( const Node & in_node, output_state & state ) {
      m_current++;
      {
	unsigned int new_percent = ceil( static_cast<double>(m_current)*100./static_cast<double>(m_total) );
	if ( new_percent != m_last_percent ) {
	  m_last_percent = new_percent;
	  std::cerr << new_percent << "% finished" << std::endl;
	}
      }
      if ( dynamic_cast<const ContentNode*>( &in_node ) != NULL ) {
	const ContentNode & content = dynamic_cast<const ContentNode &>( in_node );
	state.put_text( content.get_content() );
      } else {
	if ( dynamic_cast<const TextNode*>( &in_node ) != NULL ) {
	  const TextNode & text = dynamic_cast<const TextNode &>( in_node );
	  if ( text.is_white_space() ) {
	    return;
	  }
	} else if ( dynamic_cast<const Element*>( &in_node ) != NULL ) {
	  const Element & element = dynamic_cast<const Element&>( in_node );
	  output_state * out = NULL;
	  string name = element.get_name();
	  string label = element.get_attribute_value( "label" );
	  if ( name == "b" ) {
	    out = state.bold();
	  } else if ( name == "math" ) {
	    out = state.math();
	  } else if ( name == "equation" ) {
	    out = state.equation(label);
	  } else if ( name == "figure" ) {
	    out = state.figure(label);
	  } else if ( name == "image" ) {
	    string filename = element.get_attribute_value( "src" );
	    state.image(filename);
	  } else if ( name == "caption" ) {
	    out = state.caption();
	  } else if ( name == "plot" ) {
	    out = state.plot(label);
	  } else if ( name == "br" ) {
	    state.newline();
	  } else if ( name == "np" ) {
	    state.new_paragraph();
	  } else if ( name == "table" ) {
	    out = state.table();
	  } else if ( name == "tr" ) {
	    out = state.table_row();
	  } else if ( name == "ref" ) {
	    state.reference(label);
	  } else if ( name == "td" ) {
	    out = state.table_cell();
	  } else if ( name == "cite" ) {
	    state.cite(element.get_attribute_value( "id" ));
	  } else if ( ( name == "section" ) || ( name == "subsection" ) || ( name == "subsubsection" ) ) {
	    string section_name = element.get_attribute_value( "name" );
	    if ( section_name.length() == 0 ) {
	      throw runtime_error( "Sections must have a name attribute" );
	    }
	    unsigned int level;
	    if ( name == "subsection" ) {
	      level = 1;
	    } else if ( name == "subsubsection" ) {
	      level = 2;
	    } else {
	      level = 0;
	    }
	    out = state.section( section_name, level, label );
	  } else if ( name == "chapter" ) {
	    string section_name = element.get_attribute_value( "name" );
	    if ( section_name.length() == 0 ) {
	      throw runtime_error( "Sections must have a name attribute" );
	    }
	    out = state.chapter( section_name, label );
	  } else {
	    stringstream ss;
	    ss << "Unknown element with name \"" << name << "\" found!" << endl;
	    throw runtime_error( ss.str().c_str() );
	  }
	  if ( out != NULL ) {
	    Node::NodeList list = in_node.get_children();
	    for ( Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter ) {
	      if ( *iter != NULL ) {
		parse_node( **iter, *out );
	      }
	    }
	    out->finish();
	    delete out;
	  }
	}
      }
    }
  };

  void parse_file( bool do_html, istream & input_stream, const std::string & output_path ) {
    DomParser parser;
    parser.set_substitute_entities( true );
    parser.parse_stream( input_stream );
    if ( parser ) {
      /* if succesfull create output */
      const Element * rootNode = parser.get_document()->get_root_node();
      if ( rootNode == NULL ) {
	throw runtime_error( "get_root_node() failed" );
      }
      
      output_builder * b;
      std::ofstream * outfile = NULL;
      if ( do_html ) {
	b = new html_builder( output_path );
      } else {
	outfile = new std::ofstream(output_path.c_str());
	b = new latex_builder( *outfile );
      }

      /* do stuff */
      {	
	const Element & root_in = dynamic_cast<const Element &>( *rootNode );
	if ( root_in.get_name() != "document" ) {
	  throw runtime_error( "root node must be document" );
	}
	unsigned int total_elements = count_total_xml_elements( root_in );

	output_state * s = b->create_root();
	Node::NodeList list = root_in.get_children();
	{
	  NodeParser nparser(total_elements);
	  for ( Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter ) {
	    if ( *iter != NULL ) {
	      nparser.parse_node( **iter, * s );
	    }
	  }
	}
	s->finish();
	delete s;
      }
      delete b;
      if ( outfile != NULL ) {
	delete outfile;
      }
    }
  }
  map<string, string> gSymmap;
}

int main( int argc, char * argv[] ) {
  bool keep_text;
  string input_file_path;
  bool input_file_is_cin;
  string output_file_path;
  bool output_file_is_cout;
  bool output_html;

  g_type_init();

  /* init symbol map */
  {
    for ( unsigned int i=0; xml2epub::kSymmap[i<<1]!=NULL; ++i ) {
      xml2epub::gSymmap[xml2epub::kSymmap[i<<1]] = xml2epub::kSymmap[(i<<1)+1];
    }
  }

  xml2epub::parse_cmdline_args( argc, argv, keep_text, input_file_path, input_file_is_cin,
				output_file_path, output_file_is_cout, output_html );
  istream * in_stream = &cin;

  if ( input_file_is_cin == false ) {
    in_stream = new ifstream( input_file_path.c_str() );
    if ( !(*in_stream) ) {
      cerr << "Unable to open file \"" << input_file_path << "\" for input!" << endl;
      return -1;
    }
  }

  xml2epub::parse_file( output_html, *in_stream, output_file_path );
  
  if ( input_file_is_cin == false ) {
    delete in_stream;
  }

  return 0;
}
