NAME	= webserv-tester
CC		= g++-10
CFLAGS	= -std=c++2a
SRC_PATH= srcs/
OBJ_PATH= objs/
INC_PATH= incs/

MODULES=	Socket
SRC_DIR=	$(addprefix $(SRC_PATH),$(MODULES))
INC_DIR=	$(addprefix $(INC_PATH),$(MODULES))
OBJ_DIRS=	$(addprefix $(OBJ_PATH),$(MODULES))

SRC=	$(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
SRC+= $(SRC_PATH)main.cpp

INC= -I incs/ $(foreach sdir,$(INC_DIR),-I $(wildcard $(sdir))/)
OBJ= $(patsubst $(SRC_PATH)%.cpp,$(OBJ_PATH)%.o,$(SRC))
DEF= \033[0m
RED= \033[31;1m
GRN= \033[32;1m
YEL= \033[33;1m

.PHONY: all re clean fclean

all: $(NAME)

$(NAME): $(OBJ)
	@echo "$(RED)[$(NAME)] : $(DEF)Compiling..."
	@$(CC) $(CFLAGS) $(INC) -o $(NAME) $(OBJ)
	@echo "$(RED)[$(NAME)] : $(DEF)Compilation $(GRN)[OK]$(DEF)"

$(OBJ_PATH)%.o: $(SRC_PATH)%.cpp
	@mkdir -p $(OBJ_DIRS) 2> /dev/null || true
	@echo "$(RED)[$(NAME)] :$(DEF) Compiling $@"
	@$(CC) $(CFLAGS) $(INC) -o $@ -c $<

clean:
	@rm -rf $(OBJ_PATH)
	@echo "$(RED)[$(NAME)] : $(DEF)Cleaning $(GRN)[OK]$(DEF)"

fclean: clean
	@rm -rf $(NAME) client
	@echo "$(RED)[$(NAME)] : $(DEF)Full Cleaning $(GRN)[OK]$(DEF)"

re: fclean all