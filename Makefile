
NAME := myLittleWebserv
CLIENT := myLittleClient

OBJ_DIR := objs
SRC_DIR := server
LOG_DIR := log_files
TMP_DIR := temp

SRC :=	main.cpp\
		Router.cpp\
		VirtualServer.cpp\
		Log.cpp\
		Config.cpp\
		Event.cpp\
		EventHandler.cpp\
		HttpRequest.cpp\
		HttpResponse.cpp\
		CgiResponse.cpp\
		FileManager.cpp\
		syscall.cpp\
		ResponseFactory.cpp\
		Storage.cpp

OBJ := $(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o));

CXXFALGS = -std=c++98 -Wall -Werror -Wextra -fsanitize=address -g
LDFALGS  = -fsanitize=address -g
ifeq ($(fsanitize), no)
CXXFALGS = -std=c++98 -Wall -Werror -Wextra
LDFALGS  = 
endif

INCS := -I ./$(SRC_DIR)/Router\
		-I ./$(SRC_DIR)/Log\
		-I ./$(SRC_DIR)/abstract\
		-I ./$(SRC_DIR)/Config\
		-I ./$(SRC_DIR)/EventHandler\
		-I ./$(SRC_DIR)/VirtualServer\
		-I ./$(SRC_DIR)/HttpRequest\
		-I ./$(SRC_DIR)/HttpResponse\
		-I ./$(SRC_DIR)/CgiResponse\
		-I ./$(SRC_DIR)/FileManager\
		-I ./$(SRC_DIR)/Storage\
		-I ./$(SRC_DIR)/ResponseFactory\
		-I ./$(SRC_DIR)/syscall


ifeq ($(shell uname), Linux)
LIBRARY = -L./lib -lkqueue -lpthread
INCS += -I./lib/
endif

VPATH := $(shell ls -R)

all : directories $(NAME) $(CLIENT)

directories : $(OBJ_DIR) $(LOG_DIR) $(TMP_DIR)

$(OBJ_DIR):
	mkdir $@

$(LOG_DIR):
	mkdir $@

$(TMP_DIR):
	mkdir -p ${HOME}/goinfre/$@
	ln -s ${HOME}/goinfre/$@ $@

$(NAME) : $(OBJ)
	$(CXX) -o $@ $^ $(LDFALGS) $(LIBRARY)

$(OBJ_DIR)/%.o:%.cpp
	$(CXX) -c -o $@ $^ $(INCS) $(CXXFALGS)

clean-log:
	rm -rf $(LOG_DIR)/*.log

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(TMP_DIR)
	rm -rf $(LOG_DIR)/*

fclean: clean
	rm -rf $(CLIENT)
	rm -rf $(NAME)
	rm -rf *.out
	rm -rf *.dSYM
	rm -rf temp/*

re: fclean all

$(CLIENT): client/Client.cpp
	$(CXX) -o $@ $^ $(CXXFALGS)





