#include <stdexcept>
#include "builder.hh"

using namespace std;

namespace xml2epub {
  output_state * output_state::equation( const std::string & label ) {
    throw runtime_error( "equation statement unsupported in this state" );
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
