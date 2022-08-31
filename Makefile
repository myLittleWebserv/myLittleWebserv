NAME := myLittleWebserv
CLIENT := myLittleClient

OBJ_DIR := objs
SRC_DIR := server
LOG_DIR := log_files

SRC :=	main.cpp\
		Router.cpp\
		VirtualServer.cpp\
		Log.cpp\
		Config.cpp\
		EventHandler.cpp\
		HttpRequest.cpp\
		HttpResponse.cpp\
		Storage.cpp\
		FileManager.cpp

OBJ := $(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o));


CXXFALGS += -std=c++98 -Wall -Werror -Wextra -fsanitize=address -g 
LDFALGS  += -fsanitize=address -g

INCS := -I ./$(SRC_DIR)/Router\
		-I ./$(SRC_DIR)/Log\
		-I ./$(SRC_DIR)/Config\
		-I ./$(SRC_DIR)/EventHandler\
		-I ./$(SRC_DIR)/VirtualServer\
		-I ./$(SRC_DIR)/HttpRequest\
		-I ./$(SRC_DIR)/HttpResponse\
		-I ./$(SRC_DIR)/CgiResponse


ifeq ($(shell uname), Linux)
LIBRARY = -L./lib -lkqueue -lpthread
INCS += -I./lib/
endif

VPATH := $(shell ls -R)

all : directories $(NAME) $(CLIENT)

directories : $(OBJ_DIR) $(LOG_DIR)

$(OBJ_DIR):
	mkdir $@

$(LOG_DIR):
	mkdir $@

$(NAME) : $(OBJ)
	$(CXX) -o $@ $^ $(LDFALGS) $(LIBRARY)

$(OBJ_DIR)/%.o:%.cpp
	$(CXX) -c -o $@ $^ $(INCS) $(CXXFALGS)

clean-log:
	rm -rf $(LOG_DIR)/*.log

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(LOG_DIR)/*

fclean: clean
	rm -rf $(CLIENT)
	rm -rf $(NAME)
	rm -rf *.out
	rm -rf *.dSYM

re: fclean all

$(CLIENT): client/Client.cpp
	$(CXX) -o $@ $^





