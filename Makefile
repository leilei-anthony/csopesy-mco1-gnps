CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
SRCDIR = src
OBJDIR = obj
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TARGET = csopesy

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(CXXFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	if not exist $(OBJDIR) mkdir $(OBJDIR)

clean:
	del /Q $(OBJDIR)\*.o
	rmdir /S /Q $(OBJDIR)
	del /Q $(TARGET).exe

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

.PHONY: install