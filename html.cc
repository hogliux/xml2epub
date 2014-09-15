#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <libxml++/libxml++.h>
#include <tidy.h>
#include <buffio.h>
#include <unistd.h>

#include "html.hh"
#include "latex.hh"
#include "plot.hh"
#include "latex2util.hh"
#include "symmap.hh"

using namespace xmlpp;
using namespace std;

namespace xml2epub {
  
  class html_math_state : public html_state {
  private:
    stringstream m_ss;
  private:
    void add_text_chunk( xmlpp::Element & xml_node, string & chunk, bool is_italic ) {
      if ( chunk.size() > 0 ) {
	if ( is_italic ) {
	  Element * new_node = xml_node.add_child( "i" );
	  new_node->add_child_text( chunk );
	} else {
	  xml_node.add_child_text( chunk );
	}
	chunk = "";
      }
    }
    
    void add_child_with_italic( xmlpp::Element & xml_node, const string & text ) {
      if ( text.size() != 0 ) {
	bool is_italic = false;
	string temp;
	for ( string::const_iterator it=text.begin(); it!=text.end(); ++it ) {
	  bool new_is_italic = ( ( ( *it >= 'a' ) && ( *it <= 'z' ) ) || ( ( *it >= 'A' ) && ( *it <= 'Z' ) ) );
	  if ( new_is_italic != is_italic ) {
	    add_text_chunk( xml_node, temp, is_italic );
	  }
	  temp += *it;
	  is_italic = new_is_italic;
	}
	add_text_chunk( xml_node, temp, is_italic );
      }
    }
  public:
    html_math_state( html_builder & root, html_state & parent, xmlpp::Element & xml_node ) 
      : html_state( root, parent, xml_node ){
    }

    virtual ~html_math_state() {
    }

    output_state * bold() {
      throw runtime_error( "can't use bold xml tag in latex math" );
    }

    output_state * math() {
      throw runtime_error( "can't use math xml tag in latex math" );
    }

    void newline() {
      throw runtime_error( "can't use newline in math" );
    }

    output_state * section( const std::string & section_name, unsigned int level ) {
      throw runtime_error( "can't use section xml tag in latex math" );
    }

    output_state * plot() {
      throw runtime_error( "can't use plot xml tag in latex math" );
    }

    void put_text( const string & str ) {
      m_ss << str;
    }

    void finish() {
      /* check if the latex string can just be converted to pure unicode text */
      {
	string math(m_ss.str());
	string result;
	string tag;
	bool parse_tag = false;
	for ( string::const_iterator it=math.begin(); it!=math.end(); ++it ) {
	  if ( parse_tag == true ) {
	    if ( ( ( *it >= 'a' ) && ( *it <= 'z' ) ) || 
		 ( ( *it >= 'A' ) && ( *it <= 'Z' ) ) ) {
	      tag += * it;
	    } else {
	      parse_tag = false;
	      map<string, string>::const_iterator jt;
	      if ( ( jt = gSymmap.find( tag ) ) != gSymmap.end() ) {
		result += jt->second;
	      } else {
		result += '\\';
		result += tag;
	      }
	      tag = "";
	    }
	  }
	  if ( parse_tag == false ) {
	    if ( *it == '\\' ) {
	      parse_tag = true;	      
	      tag = "";
	    } else {
	      result += *it;
	    }
	  }
	}
	if ( result.find( '\\' ) == string::npos ) {
	  /* filter out spaces */
	  string old = result;
	  result = "";
	  for ( string::const_iterator it=old.begin(); it!=old.end(); ++it ) {
	    if ( *it != ' ' ) {
	      result += *it;
	    }
	  }
	  for ( size_t q = (result.find('^')==string::npos?result.find('_'):result.find('^'));
		q != string::npos; q = (result.find('^')==string::npos?result.find('_'):result.find('^')) ) {
	    bool is_super = ( result[q] == '^' );
	    if ( q != 0 ) {
	      add_child_with_italic( m_xml_node, result.substr( 0, q ) );
	      result = result.substr( q );
	    }
	    if ( result.size() != 0 ) {
	      string script;
	      result = result.substr( 1 );
	      if ( result[0] == '{' ) {
		result = result.substr( 1 );
		size_t p;
		if ( ( p = result.find('}') ) != string::npos ) {
		  script = result.substr( 0, p );
		  result = result.substr( p+1 );
		} else {
		  script = result;
		  result = "";
		}
	      } else {
		script = result[0];
		result = result.substr( 1 );
	      }
	      Element * new_node;
	      if ( is_super ) {
		new_node = m_xml_node.add_child( "sup" );
	      } else {
		new_node = m_xml_node.add_child( "sub" );
	      }
	      add_child_with_italic( * new_node, script );
	    }
	  }
	  /* string successfully replaced by unicode */
	  add_child_with_italic( m_xml_node, result );
	  return;
	}
      }
      string latex_string;
      {
	stringstream ss;
	ss << "$" << m_ss.str() << "$";
	latex_string = ss.str();
      }
      system( "mkdir -p images" );
      string file_name;
      {
	stringstream ss;	
	ss << getpid() << "_" << random();
	file_name = ss.str();
      }
      Element * new_node = m_xml_node.add_child( "img" );
      string img_url;
      {
	stringstream ss;
	ss << "images/" << file_name << ".svg";
	img_url = ss.str();
      }
      {
	ofstream svg_file( img_url.c_str() );
	{
	  stringstream iss( latex_string );
	  latex2svg( iss, svg_file );
	}
      }
      new_node->set_attribute( string("src"), img_url );
    }

  };

  class html_plot_state : public html_state {
  private:
    stringstream m_data;
  public:
    html_plot_state( html_builder & root, html_state & parent, xmlpp::Element & xml_node ) 
      : html_state( root, parent, xml_node ) {
    }
    
    virtual ~html_plot_state() {
    }

    output_state * bold() {
      throw runtime_error( "can't use bold xml tag in plot" );
    }

    void newline() {
      throw runtime_error( "can't use newline in plot" );
    }

    output_state * math() {
      throw runtime_error( "can't use math xml tag in plot" );
    }

    output_state * section( const std::string & section_name, unsigned int level ) {
      throw runtime_error( "can't use section xml tag in plot" );
    }

    output_state * plot() {
      throw runtime_error( "can't use plot xml tag in plot" );
    }    

    void put_text( const string & str ) {
      m_data << str;
    }

    void finish() {
      string image_file_path;
      {
	stringstream ss;
	ss << "images/" << getpid() << "_" << random() << ".svg";
	image_file_path = ss.str();
      }
      system( "mkdir -p images" );
      {
	ofstream svg_file( image_file_path.c_str() );
	if ( !svg_file ) {
	  throw runtime_error( "Unable to create image file" );
	}
	parse_plot( m_data.str(), svg_file, true );
      }
      Element * paragraph = m_xml_node.add_child( "p" );
      Element * new_node = paragraph->add_child( "img" );
      new_node->set_attribute( string("src"), image_file_path );
    }
  };

  html_state::html_state( html_builder & root, html_state & parent, Element & xml_node ) 
    : m_root( root ), m_parent( parent ), m_xml_node( xml_node ) {
  }

  html_state::html_state( html_builder & root, Element & xml_node ) :
    m_root( root ), m_parent( * this ), m_xml_node( xml_node ) {}

  html_state::~html_state() {
    if ( &m_parent == this ) {
      /* i am root */
      m_root.m_root = NULL;
    } else {
      vector<html_state*>::iterator it;
      if ( ( it = find( m_parent.m_children.begin(), m_parent.m_children.end(), this ) )
	   != m_parent.m_children.end() ) {
	m_parent.m_children.erase( it );
      }
    }
    /* delete all my children */
    if ( m_children.begin() != m_children.end() ) {
      cerr << "Warning: there are still children that have not been deleted" << endl;
      vector<html_state*> copy( m_children );
      m_children.clear();
      for ( vector<html_state*>::iterator it = copy.begin(); it != copy.end(); ++it ) {
	if ( *it != NULL ) {
	  delete *it;
	}
      }
    }
  }

  void html_state::put_text( const string & str ) {
    m_xml_node.add_child_text( str );
  }

  void html_state::newline() {
    m_xml_node.add_child( "br" );
  }

  output_state * html_state::section( const std::string & section_name, unsigned int level ) {
    string html_section_element_name;
    {
      stringstream ss;
      ss << "h" << ( level + 1 );
      html_section_element_name = ss.str();
    }
    Element * header_node = m_xml_node.add_child( html_section_element_name );
    if ( header_node == NULL ) {
      throw runtime_error( "add_child() failed" );
    }
    header_node->add_child_text( section_name );
    Element * new_node = m_xml_node.add_child( "div" );
    if ( new_node == NULL ) {
      throw runtime_error( "add_child() failed" );
    }
    {
      stringstream ss;
      ss << "sec:" << section_name;
      new_node->set_attribute( string("id"), ss.str() );
    }
    html_state * retval = new html_state( m_root, * this, * new_node );
    m_children.push_back( retval );
    return retval;
  }

  output_state * html_state::bold() {
    Element * new_node = m_xml_node.add_child( "b" );
    if ( new_node == NULL ) {
      throw runtime_error( "add_child() failed" );
    }
    html_state * retval = new html_state( m_root, * this, * new_node );
    m_children.push_back( retval );
    return retval;
  }

  output_state * html_state::math() { 
    html_state * retval = new html_math_state( m_root, * this, m_xml_node );
    m_children.push_back( retval );
    return retval;
  }

  output_state * html_state::plot() { 
    html_state * retval = new html_plot_state( m_root, * this, m_xml_node );
    m_children.push_back( retval );
    return retval;
  }

  void html_state::finish() {
    if ( &m_parent == this ) {
      string data;
      data = m_root.out_doc.write_to_string(  );
#if 1
      TidyDoc tdoc = tidyCreate();
      if ( tidyOptSetBool( tdoc, TidyXhtmlOut, yes ) == false ) {
	throw runtime_error( "tidyOptSetBool failed" );
      }
      int rc=-1;
      TidyBuffer errbuf;
      tidyBufInit( &errbuf );
      
      rc = tidySetErrorBuffer( tdoc, &errbuf );
      if ( rc < 0 ) {
	throw runtime_error( "tidySetErrorBuffer failed" );
      }
      rc = tidyParseString( tdoc, data.c_str() );
      if ( rc < 0 ) {
	throw runtime_error( "tidyParseString failed" );
      }
      rc = tidyCleanAndRepair( tdoc );
      if ( rc < 0 ) {
	throw runtime_error( "tidyCleanAndRepair failed" );
      }
      TidyBuffer output_buffer;
      tidyBufInit( &output_buffer );
      rc = tidySaveBuffer( tdoc, &output_buffer );
      if ( rc < 0 ) {
	throw runtime_error( "tidySaveBuffer failed" );
      }
      string out_buffer( (const char *)output_buffer.bp );
      tidyBufFree( &output_buffer );
      tidyBufFree( &errbuf );
      tidyRelease( tdoc );
      m_root.m_out << out_buffer;
#else
      m_root.m_out << data;
#endif
    }
  }
    
  html_builder::html_builder( ostream & output_stream ) 
    : out_root( out_doc.create_root_node( "html" ) ), m_out( output_stream ),
      m_root( NULL ) {
    if ( out_root == NULL ) {
      throw runtime_error( "create_root_node failed" );
    }
  }
    
  html_builder::~html_builder() {
    if ( m_root != NULL ) {
      delete m_root;
    }
  }

  output_state * html_builder::create_root() {
    if ( m_root != NULL ) {
      delete m_root;
    }
    m_root = new html_state( *this, * out_root );
    return m_root;
  }

}
