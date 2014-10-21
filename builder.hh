#include <string>
#pragma once

namespace xml2epub {
  class output_state {
  public:
    virtual ~output_state() {}
    virtual void put_text( const std::string & str );
    virtual void newline();
    virtual void new_paragraph();
    virtual output_state * bold();
    virtual output_state * math();
    virtual output_state * equation( const std::string & label );

    virtual output_state * table();
    virtual output_state * table_row();
    virtual output_state * table_cell();
    virtual void reference( const std::string & label );
    virtual void cite( const std::string & id );

    virtual output_state * section( const std::string & section_name, unsigned int level, const std::string & label );  
    virtual output_state * chapter( const std::string & chapter_name, const std::string & label );
    virtual output_state * plot( const std::string & label );
    virtual output_state * figure( const std::string & label );
    virtual output_state * caption( );
    virtual void image( const std::string & filename );
    virtual void finish() = 0;
  };

  class output_builder {
  public:
    virtual ~output_builder() {}
    virtual output_state * create_root() = 0;
    
  };

}
