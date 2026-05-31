CXX			=	g++
CXXFLAGS	=	-Wall -Wextra -Werror -std=c++17 -Iinclude -Isrc -MMD -MP
LDFLAGS		=

NAME		=	plazza

GREEN		=	\033[1;32m
CYAN		=	\033[1;36m
RED			=	\033[1;31m
RESET		=	\033[0m

SRC			=	src/main.cpp							\
				src/cli/Args.cpp						\
				src/concurrency/Mutex.cpp				\
				src/concurrency/ConditionVar.cpp		\
				src/concurrency/Thread.cpp				\
				src/concurrency/Process.cpp				\
				src/pizza/Pizza.cpp						\
				src/pizza/PizzaFactory.cpp				\
				src/pizza/Margarita.cpp					\
				src/pizza/Regina.cpp					\
				src/pizza/Americana.cpp					\
				src/pizza/Fantasia.cpp					\
				src/parser/Lexer.cpp					\
				src/parser/Parser.cpp					\
				src/ipc/Serializer.cpp					\
				src/ipc/IpcChannel.cpp					\
				src/ipc/Messages.cpp					\
				src/ipc/NamedPipeChannel.cpp			\
				src/kitchen/Stock.cpp					\
				src/kitchen/ThreadPool.cpp				\
				src/kitchen/Cook.cpp					\
				src/kitchen/Restocker.cpp				\
				src/kitchen/Capacity.cpp				\
				src/kitchen/IdleMonitor.cpp				\
				src/kitchen/Kitchen.cpp					\
				src/reception/Shell.cpp					\
				src/reception/KitchenHandle.cpp			\
				src/reception/Dispatcher.cpp			\
				src/reception/Spawner.cpp				\
				src/reception/ChildReaper.cpp			\
				src/reception/SignalHandler.cpp			\
				src/reception/Reception.cpp				\
				src/logging/Logger.cpp					\
				src/reception/StatusCommand.cpp			\

OBJ			=	$(SRC:.cpp=.o)
DEPS		=	$(OBJ:.o=.d) $(TESTS_OBJ:.o=.d)

LIB_SRC		=	$(filter-out src/main.cpp,$(SRC))
LIB_OBJ		=	$(LIB_SRC:.cpp=.o)

TESTS_SRC	=	tests/common/test_bootstrap.cpp			\
				tests/common/test_errors.cpp			\
				tests/cli/test_args.cpp					\
				tests/concurrency/test_mutex.cpp		\
				tests/concurrency/test_condvar.cpp		\
				tests/concurrency/test_thread.cpp		\
				tests/concurrency/test_process.cpp		\
				tests/pizza/test_factory.cpp			\
				tests/parser/test_lexer.cpp				\
				tests/parser/test_parser.cpp			\
				tests/ipc/test_serializer.cpp			\
				tests/ipc/test_named_pipe.cpp			\
				tests/kitchen/test_stock.cpp			\
				tests/kitchen/test_threadpool.cpp		\
				tests/kitchen/test_cook.cpp				\
				tests/kitchen/test_kitchen_state.cpp	\
				tests/kitchen/test_kitchen_e2e.cpp		\
				tests/reception/test_shell.cpp			\
				tests/reception/test_dispatcher.cpp		\
				tests/reception/test_e2e.cpp			\
				tests/reception/test_shutdown.cpp		\
				tests/logging/test_logger.cpp			\
				tests/reception/test_status.cpp			\

TESTS_OBJ	=	$(TESTS_SRC:.cpp=.o)
TESTS_BIN	=	tests/unit_tests
TESTS_LDFLAGS	=	-lcriterion

all:	$(NAME)

$(NAME):	$(OBJ)
	@printf "$(GREEN)Linking $(NAME)$(RESET)\n"
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o:	%.cpp
	@printf "$(CYAN)Compiling $<$(RESET)\n"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TESTS_BIN):	$(LIB_OBJ) $(TESTS_OBJ)
	@printf "$(GREEN)Linking $(TESTS_BIN)$(RESET)\n"
	$(CXX) -o $@ $^ $(LDFLAGS) $(TESTS_LDFLAGS)

tests_run:	$(TESTS_BIN)
	@printf "$(CYAN)Running tests$(RESET)\n"
	./$(TESTS_BIN)

clean:
	@printf "$(RED)Cleaning objects$(RESET)\n"
	rm -f $(OBJ) $(TESTS_OBJ) $(DEPS)

fclean:	clean
	@printf "$(RED)Cleaning binaries$(RESET)\n"
	rm -f $(NAME) $(TESTS_BIN) plazza.log

re:	fclean all

.PHONY:	all clean fclean re tests_run

-include $(DEPS)
