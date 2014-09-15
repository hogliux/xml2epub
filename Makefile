CXX=g++
XML_CFLAGS=$(shell pkg-config libxml++-2.6 --cflags)
POPPLER_CFLAGS=$(shell pkg-config poppler-glib --cflags)
TIDY_CFLAGS=$(shell pkg-config libtidy --cflags)
CFLAGS=-O0 -g $(XML_CFLAGS) $(POPPLER_CFLAGS) $(TIDY_CFLAGS)
XML_LDFLAGS=$(shell pkg-config libxml++-2.6 --libs)
POPPLER_LDFLAGS=$(shell pkg-config poppler-glib --libs)
TIDY_LDFLAGS=$(shell pkg-config libtidy --libs)
LDFLAGS=-L/home/fr810/Documents/LocalLinux/lib -lboost_program_options $(XML_LDFLAGS) $(POPPLER_LDFLAGS) $(TIDY_LDFLAGS) -Wl,-rpath,/data/users/fr810/LocalLinux/lib
CXXFLAGS=

TARGET=xml2epub

SRC=main.cc html.cc latex.cc plot.cc latex2util.cc symmap.cc builder.cc
OBJ=$(SRC:.cc=.o)
DEP=$(SRC:.cc=.d)

$(TARGET) :  $(OBJ)
	$(CXX) $(CFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.d : %.cc
	set -e; rm -f $@; \
	echo -n "./" > $@.$$$$; \
	$(CXX) -MM $(CFLAGS) $(CXXFLAGS) $< >> $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

include $(DEP)

%.o : %.cc
	$(CXX) -c -o $@ $(CFLAGS) $(CXXFLAGS) $<

clean :
	rm -rf $(OBJ)
	rm -rf $(DEP)
	rm -rf $(TARGET)
