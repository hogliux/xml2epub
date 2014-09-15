#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <string>
#include <libxml++/libxml++.h>

using namespace std;
using namespace xmlpp;

int main() {
  ifstream symbin( "symbols.dat" );
  ofstream symbou( "symmap.dat" );
  string symbol_name;
  while ( getline( symbin, symbol_name ) ) {
    cout << "Converting symbol \"" << symbol_name << "\"...   ";
    cout.flush();
    symbou << symbol_name << " ";
    unlink( "/tmp/symbol.tex" );
    unlink( "/tmp/symbol.svg" );
    {
      ofstream tex_file( "/tmp/symbol.tex" );
      tex_file << "\\documentclass{minimal}" << endl;
      tex_file << "\\usepackage{fontspec}" << endl;
      tex_file << "\\usepackage{unicode-math}" << endl;
      tex_file << "\\setmathfont{STIXGeneral}" << endl;
      tex_file << "\\begin{document}" << endl;
      tex_file << "$\\" << symbol_name << "$" << endl;
      tex_file << "\\end{document}" << endl;
    }
    system( "( ( cd /tmp && xelatex symbol.tex && inkscape -z -l symbol.svg symbol.pdf && cd - ) 2>&1 ) > /dev/null" );
    {
      ifstream svg_file( "/tmp/symbol.svg" );
      if ( svg_file ) {
	DomParser parser;
	parser.set_substitute_entities( true );
	parser.parse_stream( svg_file );
	const Element * rootNode = parser.get_document()->get_root_node();
	Node::PrefixNsMap ns;
	ns["svg"] = "http://www.w3.org/2000/svg";
	if ( rootNode != NULL ) {
	  NodeSet list = rootNode->find( "//svg:tspan", ns );
	  if ( list.size() == 1 ) {
	    const Node * n = *list.begin();
	    const Element * e;
	    if ( ( e = dynamic_cast<const Element*>(n) ) != NULL ) {
	      const TextNode * t = e->get_child_text();
	      if ( t != NULL ) {
		string content = t->get_content();
		cout << content << endl;
		symbou << content << endl;
	      } else {
		cout << "failed (no textnode)!" << endl;
	      }
	    } else {
	      cout << "failed (textspan node is not element)!" << endl;
	    }
	  } else {
	    cout << "failed (found several t-spans)!" << endl;
	  }
	} else {
	  cout << "failed (no root-node)!" << endl;
	}
      } else {
	cout << "failed (xelatex failed)!" << endl;
      }
      cout << endl;
    }
  }
  return 0;
}
