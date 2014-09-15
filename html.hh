#include <string>
#include <vector>
#include <iostream>
#include <libxml++/libxml++.h>
#include "builder.hh"
#pragma once

namespace xml2epub {

  class html_root_state;
  class html_builder : public output_builder {
  private:
    std::string m_output_directory;
    friend class html_root_state;
    html_root_state * m_root;
  public:
    html_builder( const std::string & output_dir );
    virtual ~html_builder();
    output_state * create_root();
  };

}
