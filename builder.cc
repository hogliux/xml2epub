#include <stdexcept>
#include "builder.hh"

using namespace std;

namespace xml2epub {
  void output_state::put_text( const std::string & str ) {
    throw runtime_error( "text not supported in this environment" );
  }

  void output_state::newline() {
    throw runtime_error( "new line not supported in this environment" );
  }
  
  void output_state::new_paragraph() {
    throw runtime_error( "new paragraph not supported in this environment" );
  }

  output_state * output_state::bold() {
    throw runtime_error( "bold not supported in this environment" );
  }
  
  output_state * output_state::math() {
    throw runtime_error( "math not supported in this environment" );
  }

  output_state * output_state::equation( const std::string & label ) {
    throw runtime_error( "equation statement unsupported in this state" );
  }

  void output_state::reference( const std::string & label ) {
    throw runtime_error( "equation statement unsupported in this state" );
  }

  void output_state::cite( const std::string & id ) {
    throw runtime_error( "cite statement unsupported in this state" );
  }

  output_state * output_state::section( const std::string & section_name, unsigned int level, const std::string & label ) {
    throw runtime_error( "section statement unsupported in this state" );
  }

  output_state * output_state::chapter( const std::string & chapter_name, const std::string & label ) {
    throw runtime_error( "chapter statement unsupported in this state" );
  }

  output_state * output_state::plot( const std::string & label ) {
    throw runtime_error( "plot statement unsupported in this state" );
  }

  output_state * output_state::table() {
    throw runtime_error( "table statement unsupported in this state" );
  }

  output_state * output_state::table_row() {
    throw runtime_error( "row statement unsupported in this state" );
  }
  
  output_state * output_state::table_cell() {
    throw runtime_error( "cell statement unsupported in this state" );
  }

  output_state * output_state::figure( const std::string & label ) {
    throw runtime_error( "figure statement unsupported in this state" );
  }

  output_state * output_state::caption( ) {
    throw runtime_error( "caption statement only valid in figure element" );
  }
  
  void output_state::image( const std::string & filename ) {
    throw runtime_error( "image statement only valid in figure element" );
  }
}
