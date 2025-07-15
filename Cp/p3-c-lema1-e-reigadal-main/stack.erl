-module(stack).

-export([empty/0, push/2, pop/1, peek/1]).

empty() -> [].

push(Stack, Elem) -> [Elem | Stack].

pop([]) -> empty;
pop([_|Stack]) -> Stack.


peek([]) -> empty;
peek([Elem | _]) -> {ok, Elem}.
