CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra
INCLUDES = -I./stubs -I../src
BUILD_DIR = build

COVERAGE ?= 0
ifeq ($(COVERAGE),1)
  COVERAGE_FLAGS = --coverage
else
  COVERAGE_FLAGS =
endif

LIB_SRCS = ../src/RestRequest.cpp \
           ../src/SoapRequest.cpp \
           ../src/GraphQLRequest.cpp \
           ../src/GraphQLBatchRequest.cpp \
           ../src/ESP32HTTPClient.cpp \
           ../src/BufferedStreamReader.cpp

LIB_OBJS = $(patsubst ../src/%.cpp, $(BUILD_DIR)/%.o, $(LIB_SRCS))

REST_TEST_OBJ = $(BUILD_DIR)/test_rest_request.o
SOAP_TEST_OBJ = $(BUILD_DIR)/test_soap_request.o
GRAPHQL_TEST_OBJ = $(BUILD_DIR)/test_graphql_request.o

REST_EXE = $(BUILD_DIR)/unit-tests-rest
SOAP_EXE = $(BUILD_DIR)/unit-tests-soap
GRAPHQL_EXE = $(BUILD_DIR)/unit-tests-graphql

ALL_OBJS = $(LIB_OBJS) $(REST_TEST_OBJ) $(SOAP_TEST_OBJ) $(GRAPHQL_TEST_OBJ)
DEPS = $(ALL_OBJS:.o=.d)

.PHONY: all test test-rest test-soap test-graphql clean

all: $(REST_EXE) $(SOAP_EXE) $(GRAPHQL_EXE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: ../src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(REST_EXE): $(REST_TEST_OBJ) $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) $^ -o $@

$(SOAP_EXE): $(SOAP_TEST_OBJ) $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) $^ -o $@

$(GRAPHQL_EXE): $(GRAPHQL_TEST_OBJ) $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) $^ -o $@

test-rest: $(REST_EXE)
	@echo ">>> Running REST Tests <<<"
	@$(REST_EXE)

test-soap: $(SOAP_EXE)
	@echo ">>> Running SOAP Tests <<<"
	@$(SOAP_EXE)

test-graphql: $(GRAPHQL_EXE)
	@echo ">>> Running GraphQL Tests <<<"
	@$(GRAPHQL_EXE)

test: all
	@echo ">>> Running REST Tests <<<"
	@$(REST_EXE)
	@echo ""
	@echo ">>> Running SOAP Tests <<<"
	@$(SOAP_EXE)
	@echo ""
	@echo ">>> Running GraphQL Tests <<<"
	@$(GRAPHQL_EXE)

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
