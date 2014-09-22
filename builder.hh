#include <string>
#pragma once

namespace xml2epub {
  class output_state {
  public:
    virtual ~output_state() {}
    virtual void put_text( const std::string & str ) = 0;
    virtual void newline() = 0;
    virtual void new_paragraph();
    virtual output_state * bold() = 0;
    virtual output_state * math() = 0;
    virtual output_state * equation( const std::string & label );

    virtual output_state * table();
    virtual output_state * table_row();
    virtual output_state * table_cell();
    virtual void reference( const std::string & label );
    virtual void cite( const std::string & id );

    virtual output_state * section( const std::string & section_name, unsigned int level, const std::string & label ) = 0;  
    virtual output_state * chapter( const std::string & chapter_name, const std::string & label ) = 0;
    virtual output_state * plot( const std::string & label ) = 0;
    virtual void finish() = 0;
  };

  class output_builder {
  public:
    virtual ~output_builder() {}
    virtual output_state * create_root() = 0;
    
  };

}
