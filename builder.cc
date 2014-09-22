#include <stdexcept>
#include "builder.hh"

using namespace std;

namespace xml2epub {
  output_state * output_state::equation( const std::string & label ) {
    throw runtime_error( "equation statement unsupported in this state" );
  }

  void output_state::reference( const std::string & label ) {
    throw runtime_error( "equation statement unsupported in this state" );
  }

  void output_state::cite( const std::string & id ) {
    throw runtime_error( "cite statement unsupported in this state" );
  }

  void output_state::new_paragraph( ) {
    throw runtime_error( "new paragraph statement unsupported in this state" );
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
}
