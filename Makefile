NAME := myLittleWebserv

OBJ_DIR := objs

SRC :=	main.cpp\
				Router.cpp\
				VirtualServer.cpp\
				Log.cpp\
				Config.cpp\
				EventHandler.cpp

OBJ := $(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o));


CXXFALGS += -fsanitize=address -g
LDFALGS += -fsanitize=address -g

INCS := -I ./Router\
				-I ./Log\
				-I ./Config\
				-I ./EventHandler\
				-I ./VirtualServer\
				-I ./HttpRequest\
				-I ./HttpResponse\
				-I ./CgiResponse\


ifeq ($(shell uname), Linux)
LIBRARY = -L./lib -lkqueue -lpthread
INCS += -I./lib/
endif

VPATH := $(shell ls -R)

all : $(OBJ_DIR) $(NAME)

$(OBJ_DIR) :
	mkdir $@

$(NAME) : $(OBJ)
	$(CXX) -o $@ $^ $(LDFALGS) $(LIBRARY)

$(OBJ_DIR)/%.o:%.cpp
	$(CXX) -c -o $@ $^ $(INCS) $(CXXFALGS)

clean-log:
	rm -rf *.log

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)
	rm -rf *.out
	rm -rf *.dSYM

re: fclean all


