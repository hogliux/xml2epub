#include <iostream>

namespace xml2epub {

  void latex2pdf( std::istream & input, std::string & pdf_path );
  void pdf2svg( const std::string & pdf_path, std::ostream & output, double scale_factor=1.0 );
  void svg2pdf( const std::string & svg_path, std::ostream & output, double scale_factor=1.0 );
  void latex2svg( std::istream & input, std::ostream & output );
}
