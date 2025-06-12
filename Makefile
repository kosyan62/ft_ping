# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: kosyan62 <kosyan62@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/12/13 23:01:42 by kosyan62          #+#    #+#              #
#    Updated: 2024/12/13 23:01:42 by kosyan62         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = ft_ping
CC = gcc
CFLAGS = -std=gnu99
IFLAGS = -I$(INC_DIR)
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = includes

# Source files
SRC_FILES = main.c icmp_tools.c
SRC = $(addprefix $(SRC_DIR)/, $(SRC_FILES))
OBJ = $(addprefix $(OBJ_DIR)/, $(SRC_FILES:.c=.o))

# Libraries
LIBS = -lm

# Header files
HEADERS = $(INC_DIR)/ping.h

# Colors for output
GREEN = \033[0;32m
RESET = \033[0m

all: $(OBJ_DIR) $(NAME)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) -o $(NAME)
	@echo "$(GREEN)$(NAME) successfully compiled!$(RESET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	@echo "$(GREEN)Object files removed!$(RESET)"

fclean: clean
	rm -f $(NAME)
	@echo "$(GREEN)$(NAME) removed!$(RESET)"

norm:
	norminette $(SRC_DIR)/*.c $(INC_DIR)/*.h

re: fclean all

.PHONY: all clean fclean re norm