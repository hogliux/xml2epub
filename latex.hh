#include <string>
#include <vector>
#include <iostream>
#include <libxml++/libxml++.h>
#include "builder.hh"
#pragma once

namespace xml2epub {
  class latex_builder;

  class latex_state : public output_state {
  protected:
    friend class latex_builder;
    latex_builder & m_root;
    latex_state & m_parent;
    std::vector<latex_state*> m_children;
    std::ostream & m_out;
  protected:
    latex_state( latex_builder & root, latex_state & parent, std::ostream & outs );  
  public:
    virtual ~latex_state();
  
    void put_text( const std::string & str );  
    void newline();
    void new_paragraph();
    void reference( const std::string & label );
    void cite( const std::string & id );
    output_state * bold();
    output_state * math();
    output_state * equation( const std::string & label );
    output_state * section( const std::string & section_name, unsigned int level, const std::string & label );
    output_state * chapter( const std::string & chapter_name, const std::string & label );
    output_state * plot( const std::string & label );
    void finish();
  };

  class latex_builder : public output_builder {
  private:
    friend class latex_state;
    std::ostream & m_out;
    latex_state * m_root;
    bool m_minimal;
  public:
    latex_builder( std::ostream & output_stream, bool minimal = false );
    virtual ~latex_builder();
    output_state * create_root();
  };

}
