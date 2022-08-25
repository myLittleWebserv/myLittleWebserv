NAME := myLittleWebserv

OBJ_DIR := objs
LOG_DIR := LogFiles

SRC :=	main.cpp\
				Router.cpp\
				VirtualServer.cpp\
				Log.cpp\
				Config.cpp\
				EventHandler.cpp\
				HttpRequest.cpp\
				HttpResponse.cpp\
				Storage.cpp

OBJ := $(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o));


CXXFALGS += -fsanitize=address -g -std=c++11
LDFALGS += -fsanitize=address -g

INCS := -I ./Router\
				-I ./Log\
				-I ./Config\
				-I ./EventHandler\
				-I ./VirtualServer\
				-I ./HttpRequest\
				-I ./HttpResponse\
				-I ./CgiResponse


ifeq ($(shell uname), Linux)
LIBRARY = -L./lib -lkqueue -lpthread
INCS += -I./lib/
endif

VPATH := $(shell ls -R)

all : directories $(NAME)

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

fclean: clean
	rm -rf $(NAME)
	rm -rf *.out
	rm -rf *.dSYM

re: fclean all


