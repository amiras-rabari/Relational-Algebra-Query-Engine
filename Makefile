CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -g

SRC_DIR = src
TEST_DIR = src/Test
DATA_DIR = DataGenerator

QUERY_PROCESSOR_SOURCES = \
	$(SRC_DIR)/Tokenizer.cpp \
	$(SRC_DIR)/Parser.cpp \
	$(SRC_DIR)/ASTPrinter.cpp \
	$(SRC_DIR)/QueryParser.cpp \
	$(SRC_DIR)/RelationParser.cpp \
	$(SRC_DIR)/ParserContext.cpp \
	$(SRC_DIR)/SemanticValidator.cpp \
	$(SRC_DIR)/Executioner.cpp

QUERY_PROCESSOR_OBJECTS = $(QUERY_PROCESSOR_SOURCES:.cpp=.o)

.PHONY: all clean

all: Query_Processor Query_Processor_Test DataGenerator

Query_Processor: $(SRC_DIR)/main.o $(QUERY_PROCESSOR_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

Query_Processor_Test: $(TEST_DIR)/QueryTester.o $(QUERY_PROCESSOR_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

DataGenerator: $(DATA_DIR)/main.o
	$(CXX) $(CXXFLAGS) -o $@ $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@

$(TEST_DIR)/%.o: $(TEST_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@

$(DATA_DIR)/%.o: $(DATA_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o
	rm -f $(TEST_DIR)/*.o
	rm -f $(DATA_DIR)/*.o
	rm -f Query_Processor
	rm -f Query_Processor_Test
	rm -f DataGenerator
