NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude

SRC = main.cpp src/Server.cpp src/Client.cpp src/Message.cpp src/CommandHandler.cpp src/Channel.cpp \
		src/commands/ChannelCommands.cpp src/commands/MemberCommands.cpp src/commands/RegistrationCommands.cpp \
		src/commands/MessageCommands.cpp
OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re