#include <cstdlib>
#include <iomanip>
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

  class html_state : public output_state {
  protected:
    friend class html_builder;
    html_state & m_parent;
    std::vector<html_state*> m_children;
    xmlpp::Element & m_xml_node;
    const std::string & m_current_dir;
  protected:
    html_state( html_state & parent, xmlpp::Element & xml_node, const std::string & current_dir );
    html_state( xmlpp::Element & xml_node, const std::string & current_dir );
    virtual ~html_state();
  public:
    void put_text( const std::string & str );
    void newline();
    void reference( const std::string & label );
    void cite( const std::string & id );
    output_state * bold();
    output_state * math();
    output_state * equation(const std::string & label );
    output_state * section( const std::string & section_name, unsigned int level, const std::string & label );
    output_state * chapter( const std::string & chapter_name, const std::string & label );
    output_state * plot(const std::string & label);
    void finish();
  };

  
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
    html_math_state( html_state & parent, xmlpp::Element & xml_node, const std::string & current_dir ) 
      : html_state( parent, xml_node, current_dir ){
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

    output_state * section( const std::string & section_name, unsigned int level, const std::string & label ) {
      throw runtime_error( "can't use section xml tag in latex math" );
    }

    output_state * plot(const std::string & label) {
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
	bool has_unicode_represnetation = true;
	for ( size_t pos = result.find( '\\' ); pos != string::npos; pos = result.find( '\\', pos+1 ) ) {
	  if ( result.substr( pos, string( "\\mathbf{" ).size() ) != string( "\\mathbf{" ) ) {
	    has_unicode_represnetation = false;
	    break;
	  }
	}
	if ( has_unicode_represnetation ) {
	  /* filter out spaces */
	  string old = result;
	  result = "";
	  for ( string::const_iterator it=old.begin(); it!=old.end(); ++it ) {
	    if ( *it != ' ' ) {
	      result += *it;
	    }
	  }
	  /* replace \mathbf with @ symbol */
	  for ( size_t pos = result.find( '\\' ); pos != string::npos; pos = result.find( '\\' ) ) {
	    if ( result.substr( pos, string( "\\mathbf{" ).size() ) == string( "\\mathbf{" ) ) {
	      result = result.replace( pos, string( "\\mathbf{" ).size(), "@{" );
	    }
	  }
	  for ( size_t q = (result.find('^')==string::npos?(result.find('_')==string::npos?result.find('@'):result.find('_')):result.find('^'));
		q != string::npos; q = (result.find('^')==string::npos?(result.find('_')==string::npos?result.find('@'):result.find('_')):result.find('^')) ) {
	    char the_char = result[q];
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
	      if ( the_char == '^' ) {
		new_node = m_xml_node.add_child( "sup" );
		add_child_with_italic( * new_node, script );
	      } else if ( the_char == '_' ) {
		new_node = m_xml_node.add_child( "sub" );
		add_child_with_italic( * new_node, script );	      
	      } else {
		new_node = m_xml_node.add_child( "b" );
		add_text_chunk( * new_node, script, false );
	      }
	    }
	  }
	  /* string successfully replaced by unicode */
	  add_child_with_italic( m_xml_node, result );
	  return;
	}
      }
      {
	stringstream ss;
	ss << "mkdir -p " << m_current_dir << "/images";
	system(ss.str().c_str());
      }
      string latex_string;
      {
	stringstream ss;
	ss << "$" << m_ss.str() << "$";
	latex_string = ss.str();
      }
      string file_name;
      {
	stringstream ss;	
	ss << getpid() << "_" << random();
	file_name = ss.str();
      }
      Element * new_node = m_xml_node.add_child( "img" );
      string img_path;
      {
	stringstream ss;
	ss << m_current_dir << "/images/" << file_name << ".svg";
	img_path = ss.str();
      }
      {
	ofstream svg_file( img_path.c_str() );
	{
	  stringstream iss( latex_string );
	  latex2svg( iss, svg_file );
	}
      }
      string img_url;
      {
	stringstream ss;
	ss << "images/" << file_name << ".svg";
	img_url = ss.str();
      }
      new_node->set_attribute( string("src"), img_url );
    }

  };

  class html_plot_state : public html_state {
  private:
    stringstream m_data;
    string m_label;
  public:
    html_plot_state( html_state & parent, xmlpp::Element & xml_node, const std::string & label, const std::string & current_dir ) 
      : html_state( parent, xml_node, current_dir ), m_label(label) {
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

    output_state * section( const std::string & section_name, unsigned int level, const std::string & label ) {
      throw runtime_error( "can't use section xml tag in plot" );
    }

    output_state * plot(const std::string & label) {
      throw runtime_error( "can't use plot xml tag in plot" );
    }    

    void put_text( const string & str ) {
      m_data << str;
    }

    void finish() {
      string file_name;
      {
	stringstream ss;
	ss << getpid() << "_" << random() << ".svg";
	file_name = ss.str();
      }
      string image_file_path;
      {
	stringstream ss;
	ss << m_current_dir << "/images/" << file_name;
	image_file_path = ss.str();
      }
      {
	stringstream ss;
	ss << "mkdir -p " << m_current_dir << "/images";
	system(ss.str().c_str());
      }
      {
	ofstream svg_file( image_file_path.c_str() );
	if ( !svg_file ) {
	  throw runtime_error( "Unable to create image file" );
	}
	parse_plot( m_data.str(), svg_file, true );
      }
      string image_url;
      {
	stringstream ss;
	ss << "images/" << file_name;
	image_url = ss.str();
      }
      Element * paragraph = m_xml_node.add_child( "p" );
      if ( m_label.size() != 0 ) {
	paragraph->set_attribute(string("id"), m_label);
      }
      Element * new_node = paragraph->add_child( "img" );
      new_node->set_attribute( string("src"), image_url );
    }

  };

  class html_equation_state : public html_state {
  private:
    stringstream m_data;
    string m_label;
  public:
    html_equation_state( html_state & parent, xmlpp::Element & xml_node, const std::string & label, const std::string & current_dir ) 
      : html_state( parent, xml_node, current_dir ), m_label(label) {
    }
    
    virtual ~html_equation_state() {
    }

    output_state * bold() {
      throw runtime_error( "can't use bold xml tag in equation" );
    }

    void newline() {
      throw runtime_error( "can't use newline in equation" );
    }

    output_state * math() {
      throw runtime_error( "can't use math xml tag in equation" );
    }

    output_state * section( const std::string & section_name, unsigned int level ) {
      throw runtime_error( "can't use section xml tag in equation" );
    }

    output_state * equation() {
      throw runtime_error( "can't use equation xml tag in equation" );
    }    

    void put_text( const string & str ) {
      m_data << str;
    }

    void finish() {
      string file_name;
      {
	stringstream ss;
	ss << getpid() << "_" << random() << ".svg";
	file_name = ss.str();
      }
      string image_file_path;
      {
	stringstream ss;
	ss << m_current_dir << "/images/" << file_name;
	image_file_path = ss.str();
      }
      {
	stringstream ss;
	ss << "mkdir -p " << m_current_dir << "/images";
	system(ss.str().c_str());
      }
      {
	ofstream svg_file( image_file_path.c_str() );
	if ( !svg_file ) {
	  throw runtime_error( "Unable to create image file" );
	}
	parse_equation( m_data.str(), svg_file );
      }
      string image_url;
      {
	stringstream ss;
	ss << "images/" << file_name;
	image_url = ss.str();
      }
      Element * paragraph = m_xml_node.add_child( "p" );
      if ( m_label.size() != 0 ) {
	paragraph->set_attribute(string("id"), m_label);
      }
      Element * new_node = paragraph->add_child( "img" );
      new_node->set_attribute( string("src"), image_url );
    }

    void parse_equation( const std::string & eqn, std::ostream & svg_output ) {
      std::string equation(eqn);
      for ( size_t pos = equation.find("\n",0); pos != std::string::npos; pos = equation.find("\n",pos+1) ) {
	equation[pos] = ' ';
      }
      string file_name;
      {
	stringstream ss;
	ss << getpid() << "_" << random();
	file_name = ss.str();
      }
      string tex_path;
      {
	stringstream ss;
	ss << "/tmp/" << file_name << ".tex";
	tex_path = ss.str();
      }
      {
	ofstream tex_file( tex_path.c_str() );
	if ( !tex_file ) {
	  throw runtime_error( "Cannot creat tmp file" );
	}
	tex_file << "\\documentclass{minimal}" << endl;
	tex_file << "\\usepackage{fontspec}" << endl;
	tex_file << "\\usepackage{unicode-math}" << endl;
	tex_file << "\\usepackage{amsmath}" << endl;
	tex_file << "\\setmathfont{STIXGeneral}" << endl;
	tex_file << "\\begin{document}" << endl;
	tex_file << "\\begin{equation*}" << endl;
	tex_file << equation << endl;
	tex_file << "\\end{equation*}" << endl;
	tex_file << "\\end{document}" << endl;
      }

      string shell_command;
      {
	stringstream ss;
	ss << "( ( cd /tmp; xelatex " << file_name << ".tex; cd -; ) 2>&1 ) > /dev/null";
	shell_command = ss.str();
      }
      system( shell_command.c_str() );
      {
	string pdf_file;
	{
	  stringstream ss;
	  ss << "/tmp/" << file_name << ".pdf";
	  pdf_file = ss.str();
	}
	pdf2svg( pdf_file, svg_output );
      }
      {
	stringstream ss;
	ss << "rm -f /tmp/" << file_name << ".*";
	shell_command = ss.str();
      }
      system( shell_command.c_str() ); 
    }
  };
  
  html_state::html_state( html_state & parent, xmlpp::Element & xml_node, const std::string & current_dir )
    : m_parent( parent ), m_xml_node( xml_node ), m_current_dir(current_dir) {
  }
  
  html_state::html_state( xmlpp::Element & xml_node, const std::string & current_dir )
    : m_parent( * this ), m_xml_node( xml_node ), m_current_dir(current_dir) {}
  
  html_state::~html_state() {
    if ( &m_parent != this ) {
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

  void html_state::put_text( const std::string & str ) {
    m_xml_node.add_child_text( str );
  }

  void html_state::newline() {
    m_xml_node.add_child( "br" );
  }

  output_state * html_state::bold() {
    Element * new_node = m_xml_node.add_child( "b" );
    if ( new_node == NULL ) {
      throw runtime_error( "add_child() failed" );
    }
    html_state * retval = new html_state( * this, * new_node, m_current_dir );
    m_children.push_back( retval );
    return retval;
  }

  output_state * html_state::math() {
    html_state * retval = new html_math_state( * this, m_xml_node, m_current_dir );
    m_children.push_back( retval );
    return retval;
  }

  output_state * html_state::equation(const std::string & label ) {
    html_state * retval = new html_equation_state( * this, m_xml_node, label, m_current_dir );
    m_children.push_back( retval );
    return retval;
  }

  void html_state::reference( const std::string & label ) {
    //TODO
    Element * link_node = m_xml_node.add_child( string("a") );
    link_node->set_attribute( string("href"), string("#")+label );
    link_node->add_child_text( string("?") );
  }

  void html_state::cite( const std::string & id ) {
    //TODO
    Element * link_node = m_xml_node.add_child( string("a") );
    link_node->set_attribute( string("href"), string("bibliography.hmtl#")+id );
    link_node->add_child_text( string("[?]") );
  }

  output_state * html_state::section( const std::string & section_name, unsigned int level, const std::string & label ) {
    string html_section_element_name;
    {
      stringstream ss;
      ss << "h" << ( level + 2 );
      html_section_element_name = ss.str();
    }
    Element * header_node = m_xml_node.add_child( html_section_element_name );
    if ( label.size() != 0 ) {
      header_node->set_attribute(string("id"), label );
    }
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
    html_state * retval = new html_state( * this, * new_node, m_current_dir );
    m_children.push_back( retval );
    return retval;
  }

  output_state * html_state::chapter( const std::string & chapter_name, const std::string & label ) {
    throw std::runtime_error( "Chapter within chapter is not allowed -> use section" );
  }

  output_state * html_state::plot(const std::string & label) {
    html_state * retval = new html_plot_state( * this, m_xml_node, label, m_current_dir );
    m_children.push_back( retval );
    return retval;
  }
  
  void html_state::finish() {
  }

  class html_root_state;

  class html_chapter_state : public html_state {
  private:
    html_root_state & m_parent;
    xmlpp::Document * m_doc;
    std::ostream & m_out;
  protected:
    friend class html_root_state;
    /* html_chapter_state is responsible for de-allocating xml-doc!! */
    html_chapter_state( html_root_state & parent, xmlpp::Document * xml_doc, 
			std::ostream & out, const std::string & label, const std::string & current_dir ) : 
      html_state( *this, *xml_doc->create_root_node( "html" ), current_dir ), m_parent(parent), m_doc(xml_doc), m_out(out) {
      if ( label.size() != 0 ) {
	Element * head_node = m_xml_node.add_child( "head" );
	Element * title_node = head_node->add_child( "title" );
	title_node->add_child_text( label.c_str() );
	Element * h1 = m_xml_node.add_child( "h1" );
	h1->add_child_text( label.c_str() );
      }
    }
  public:
    virtual ~html_chapter_state();
    void finish() {
      string data;
      data = m_doc->write_to_string();
      /* delete the first line so that tidy will add it's own xml tag */
      data = data.substr( data.find('\n')+1 );

      TidyDoc tdoc = tidyCreate();
      if ( tidyOptSetBool( tdoc, TidyXhtmlOut, yes ) == false ) {
	throw runtime_error( "tidyOptSetBool failed" );
      }
      if ( tidyOptSetBool( tdoc, TidyXmlDecl, yes ) == false ) {
	throw runtime_error( "tidyOptSetBool failed" );
      }
      tidySetInCharEncoding( tdoc, "utf8" );
      tidySetOutCharEncoding( tdoc, "latin1" );
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
      m_out << out_buffer;
    }
  };

  class html_root_state : public output_state {
  private:
    html_builder & m_builder;
    string m_parent_directory;
    unsigned int chapter_number;
    std::vector<std::pair<html_chapter_state*, std::ofstream*> > m_chapters;
    friend class html_builder;
    html_root_state( html_builder & builder, const std::string & dir ) : m_builder(builder), m_parent_directory( dir ) {}
  public:
    virtual ~html_root_state() {
      m_builder.m_root = NULL;
      if ( m_chapters.size() != 0 ) {
	std::cerr << "WARNING: not all html chapters have been de-alloced!?!?" << std::endl;
	for ( std::vector<std::pair<html_chapter_state*, std::ofstream*> >::const_iterator it = m_chapters.begin();
	      it != m_chapters.end(); ++it ) {
	  delete it->first;
	  delete it->second;
	}
      }
    }
    void put_text( const std::string & str ) {
      for ( std::string::const_iterator it = str.begin(); it != str.end(); ++it ) {
	if ( ( *it != ' ' ) && ( *it != '\r' ) && ( *it != '\n' ) && ( *it != '\t' ) ) {
	  throw std::runtime_error("You must open a chapter before putting in text!");
	}
      }
    }
    void newline() {
      throw std::runtime_error("You must open a chapter before putting in text!");
    }
    output_state * bold() {
      throw std::runtime_error("You must open a chapter before putting in text!");
    }
    output_state * math() {
      throw std::runtime_error("You must open a chapter before putting in math!");
    }
    output_state * section( const std::string & section_name, unsigned int level, const std::string & label ) {
      throw std::runtime_error("You must open a chapter before putting in section!");
    }
    output_state * chapter( const std::string & chapter_name, const std::string & label ) {
      chapter_number++;
      string filename;
      {
	stringstream ss;
	ss << m_parent_directory << "/" << "chapter" << setw(2) << setfill('0') << chapter_number << ".html";
	filename = ss.str();
      }
      std::ofstream * outfile = new std::ofstream(filename.c_str());
      string pretty_name;
      {
	stringstream ss;
	ss << "Chapter " << chapter_number << ": " << chapter_name;
	pretty_name = ss.str();
      }
      html_chapter_state * state = new html_chapter_state(*this, new xmlpp::Document, *outfile, pretty_name, 
							  m_parent_directory);
      
      m_chapters.push_back( std::pair<html_chapter_state*, std::ofstream*>( state, outfile ) );
      return state;
    }
    output_state * plot(const std::string & label) {
      throw std::runtime_error("You must open a chapter before putting in plot!");
    }
    void finish() {
    }
  private:
    friend class html_chapter_state;
    void remove_me( html_chapter_state & chapter_state ) {
      for ( std::vector<std::pair<html_chapter_state*, std::ofstream*> >::iterator it = m_chapters.begin();
	    it != m_chapters.end(); ++it ) {
	if ( it->first == (&chapter_state) ) {
	  if ( it->second != NULL ) {
	    delete it->second;
	    it->second = NULL;
	  }
	  m_chapters.erase(it);
	  break;
	}
      }
    }
  };

  html_chapter_state::~html_chapter_state() {
    m_parent.remove_me( *this );
    delete m_doc;
  }

  html_builder::html_builder( const std::string & output_dir ) 
    : m_output_directory(output_dir), m_root(NULL) {
    {
      stringstream ss;
      ss << "rm -rf \"" << m_output_directory << "\" && mkdir -p \"" << m_output_directory << "\"";
      system( ss.str().c_str() );
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
    m_root = new html_root_state( *this, m_output_directory );
    return m_root;
  }

}
