CXX = c++
CXXFLAGS = -std=c++23 -Wall -Wextra -I./include
DEBUGFLAGS = -g -O0 -DDEBUG
RELEASEFLAGS = -O3

SRCDIR = src
OBJDIR = build
BINDIR = build

SRCS = $(filter-out $(SRCDIR)/test_socket.cpp,$(wildcard $(SRCDIR)/*.cpp))
OBJS = $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/app

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) $(OBJS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

debug: RELEASEFLAGS = $(DEBUGFLAGS)
debug: clean $(TARGET)
	lldb ./$(TARGET)

clean:
	@rm -f $(OBJS) $(TARGET)

.PHONY: all run debug clean help
