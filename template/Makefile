CC=clang
CXX=clang++

OBJDIR := build
DEPDIR := build

CXXFLAGS := $(CXXFLAGS) -O3 -g -std=c++23
CXXFLAGS := $(CXXFLAGS) -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
# CXXFLAGS := $(CXXFLAGS) -DDEBUG
# CXXFLAGS := $(CXXFLAGS) -DARENA_DEBUG 
# CXXFLAGS := $(CXXFLAGS) -DSCRATCH_DEBUG 
# CXXFLAGS := $(CXXFLAGS) -fsanitize=address 

# clang only
# CXXFLAGS := $(CXXFLAGS) -ftime-trace
 	
CFLAGS = $(CPPFLAGS)

LDFLAGS := $(LDFLAGS) -lstdc++exp 
LDFLAGS := $(LDFLAGS) -rdynamic 
LDFLAGS := $(LDFLAGS) -fsanitize=address 

DEPFLAGS= -MM -MG -MF


SRCS := main.cpp
-include core/core.mk

OBJS := $(patsubst %,$(OBJDIR)/%.o,$(basename $(SRCS)))
DEPENDS := $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS)))
OUTPUT := string

.SUFFIXES:
.PHONY: all clean run

all:$(OUTPUT)

$(OUTPUT): $(OBJS) 
	$(CXX) -o $@ $^ $(LDFLAGS) 

$(OBJDIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) -c $< $(CXXFLAGS) -o $@

$(OBJDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $< $(CFLAGS) -o $@


clean:
	@rm -r $(OBJDIR)
	@rm $(OUTPUT)

run: $(OUTPUT)
	./$(OUTPUT)

.PRECIOUS: $(DEPDIR)/%.d
.PRECIOUS: $(OBJDIR)/%.o

$(DEPDIR)/%.d:: %.c
	mkdir -p $(dir $@)
	$(CC) $(DEPFLAGS) $@ $<

$(DEPDIR)/%.d:: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(DEPFLAGS) $@ $< -MP -MT $(<:%.cpp=$(OBJDIR)/%.o)

include  $(DEPENDS)
