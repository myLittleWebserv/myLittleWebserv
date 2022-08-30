OBJ_DIR := objs

SRC := Test_EventHandler.cpp EventHandler.cpp Log.cpp
OBJ := $(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o));
TEST := Test_EventHandler.out

CXXFALGS += -fsanitize=address -g
LDFALGS += -fsanitize=address -g

INCS := -I ../\
				-I ../../Log

VPATH := $(shell ls -R ../../)

all : $(OBJ_DIR) $(TEST)

$(OBJ_DIR) :
	mkdir $@

$(TEST) : $(OBJ)
	$(CXX) -o $@ $^ $(LDFALGS)

$(OBJ_DIR)/%.o:%.cpp
	$(CXX) -c -o $@ $^ $(INCS) $(CXXFALGS)


$(OBJ_DIR)/%.o:../%.cpp
	$(CXX) -c -o $@ $^ $(INCS) $(CXXFALGS)


$(OBJ_DIR)/%.o:Log/%.cpp
	$(CXX) -c -o $@ $^ $(INCS)	$(CXXFALGS)

clean-log:
	rm -rf *.log

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(TEST)
	rm -rf *.out

re: fclean all


